/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include "ch.h"
#include "test.h"

#include "lpc43xx_cpp.hpp"

#include "portapack_shared_memory.hpp"
#include "portapack_dma.hpp"

#include "gpdma.hpp"

#include "baseband_dma.hpp"

#include "event_m4.hpp"

#include "rssi.hpp"
#include "rssi_dma.hpp"

#include "touch_dma.hpp"

#include "dsp_decimate.hpp"
#include "dsp_demodulate.hpp"
#include "dsp_fft.hpp"
#include "dsp_fir_taps.hpp"
#include "dsp_iir.hpp"

#include "block_decimator.hpp"
#include "clock_recovery.hpp"
#include "access_code_correlator.hpp"
#include "packet_builder.hpp"

#include "message_queue.hpp"

#include "utility.hpp"

#include "debug.hpp"

#include "audio.hpp"
#include "audio_dma.hpp"

#include "gcc.hpp"

#include <cstdint>
#include <cstddef>
#include <complex>
#include <array>
#include <string>
#include <bitset>

constexpr auto baseband_thread_priority = NORMALPRIO + 20;
constexpr auto rssi_thread_priority = NORMALPRIO + 10;

static float complex16_mag_squared_to_dbv_norm(const float c16_mag_squared) {
	constexpr float mag2_max = -32768.0f * -32768.0f + -32768.0f * -32768.0f;
	constexpr float mag2_log10_max = std::log10(mag2_max);
	constexpr float mag2_to_db_factor = 20.0f / 2.0f;
	return (std::log10(c16_mag_squared) - mag2_log10_max) * mag2_to_db_factor;
}

class BasebandStatsCollector {
public:
	template<typename Callback>
	void process(buffer_c8_t buffer, Callback callback) {
		samples += buffer.count;

		const size_t report_samples = buffer.sampling_rate * report_interval;
		const auto report_delta = samples - samples_last_report;
		if( report_delta >= report_samples ) {
			const auto idle_ticks = chSysGetIdleThread()->total_ticks;
			statistics.idle_ticks = (idle_ticks - last_idle_ticks);
			last_idle_ticks = idle_ticks;

			const auto baseband_ticks = chThdSelf()->total_ticks;
			statistics.baseband_ticks = (baseband_ticks - last_baseband_ticks);
			last_baseband_ticks = baseband_ticks;

			statistics.saturation = m4_flag_saturation();
			clear_m4_flag_saturation();

			callback(statistics);

			samples_last_report = samples;
		}
	}

private:
	static constexpr float report_interval { 1.0f };
	BasebandStatistics statistics;
	size_t samples { 0 };
	size_t samples_last_report { 0 };
	uint32_t last_idle_ticks { 0 };
	uint32_t last_baseband_ticks { 0 };
};

class RSSIStatisticsCollector {
public:
	template<typename Callback>
	void process(rf::rssi::buffer_t buffer, Callback callback) {
		auto p = buffer.p;
		if( p == nullptr ) {
			return;
		}

		const auto end = &p[buffer.count];
		while(p < end) {
			const uint32_t value = *(p++);

			if( statistics.min > value ) {
				statistics.min = value;
			}
			if( statistics.max < value ) {
				statistics.max = value;
			}

			statistics.accumulator += value;
		}
		statistics.count += buffer.count;

		const size_t samples_per_update = buffer.sampling_rate * update_interval;

		if( statistics.count >= samples_per_update ) {
			callback(statistics);
			statistics.accumulator = 0;
			statistics.count = 0;
			const auto value_0 = *p;
			statistics.min = value_0;
			statistics.max = value_0;
		}
	}

private:
	static constexpr float update_interval { 0.1f };
	RSSIStatistics statistics;
};

class ChannelStatsCollector {
public:
	template<typename Callback>
	void feed(buffer_c16_t src, Callback callback) {
		auto src_p = src.p;
		while(src_p < &src.p[src.count]) {
			const uint32_t sample = *__SIMD32(src_p)++;
			const uint32_t mag_sq = __SMUAD(sample, sample);
			if( mag_sq > max_squared ) {
				max_squared = mag_sq;
			}
		}
		count += src.count;

		const size_t samples_per_update = src.sampling_rate * update_interval;

		if( count >= samples_per_update ) {
			const float max_squared_f = max_squared;
			const float max_db_f = complex16_mag_squared_to_dbv_norm(max_squared_f);
			const int32_t max_db = max_db_f;
			const ChannelStatistics statistics {
				.max_db = max_db,
				.count = count,
			};
			callback(statistics);

			max_squared = 0;
			count = 0;
		}
	}

private:
	static constexpr float update_interval { 0.1f };
	uint32_t max_squared { 0 };
	size_t count { 0 };
};

class AudioStatsCollector {
public:
	template<typename Callback>
	void feed(buffer_s16_t src, Callback callback) {
		auto src_p = src.p;
		const auto src_end = &src.p[src.count];
		while(src_p < src_end) {
			const auto sample = *(src_p++);
			const uint64_t sample_squared = sample * sample;
			squared_sum += sample_squared;
			if( sample_squared > max_squared ) {
				max_squared = sample_squared;
			}
		}
		count += src.count;

		const size_t samples_per_update = src.sampling_rate * update_interval;

		if( count >= samples_per_update ) {
			const float squared_sum_f = squared_sum;
			const float max_squared_f = max_squared;
			const float squared_avg_f = squared_sum_f / count;
			const int32_t rms_db = complex16_mag_squared_to_dbv_norm(squared_avg_f);
			const int32_t max_db = complex16_mag_squared_to_dbv_norm(max_squared_f);
			const AudioStatistics statistics {
				.rms_db = rms_db,
				.max_db = max_db,
				.count = count,
			};
			callback(statistics);

			squared_sum = 0;
			max_squared = 0;
			count = 0;
		}
	}

private:
	static constexpr float update_interval { 0.1f };
	uint64_t squared_sum { 0 };
	uint32_t max_squared { 0 };
	size_t count { 0 };
};

class ChannelDecimator {
public:
	enum class DecimationFactor {
		By4,
		By8,
		By16,
		By32,
	};

	ChannelDecimator(
		DecimationFactor f
	) : decimation_factor { f }
	{
	}

	void set_decimation_factor(const DecimationFactor f) {
		decimation_factor = f;
	}

	buffer_c16_t execute(buffer_c8_t buffer) {
		auto decimated = execute_decimation(buffer);

		return decimated;
	}

private:
	std::array<complex16_t, 1024> work_baseband;

	const buffer_c16_t work_baseband_buffer {
		work_baseband.data(),
		work_baseband.size()
	};
	const buffer_s16_t work_audio_buffer {
		(int16_t*)work_baseband.data(),
		sizeof(work_baseband) / sizeof(int16_t)
	};

	//const bool fs_over_4_downconvert = true;

	dsp::decimate::TranslateByFSOver4AndDecimateBy2CIC3 translate;
	//dsp::decimate::DecimateBy2CIC3 cic_0;
	dsp::decimate::DecimateBy2CIC3 cic_1;
	dsp::decimate::DecimateBy2CIC3 cic_2;
	dsp::decimate::DecimateBy2CIC3 cic_3;
	dsp::decimate::DecimateBy2CIC3 cic_4;

	DecimationFactor decimation_factor { DecimationFactor::By32 };

	buffer_c16_t execute_decimation(buffer_c8_t buffer) {
		/* 3.072MHz complex<int8_t>[2048], [-128, 127]
		 * -> Shift by -fs/4
		 * -> 3rd order CIC: -0.1dB @ 0.028fs, -1dB @ 0.088fs, -60dB @ 0.468fs
		 *                   -0.1dB @ 86kHz,   -1dB @ 270kHz,  -60dB @ 1.44MHz
		 * -> gain of 256
		 * -> decimation by 2
		 * -> 1.544MHz complex<int16_t>[1024], [-32768, 32512] */
		const auto stage_0_out = translate.execute(buffer, work_baseband_buffer);

		//if( fs_over_4_downconvert ) {
		//	// TODO:
		//} else {
		// Won't work until cic_0 will accept input type of buffer_c8_t.
		//	stage_0_out = cic_0.execute(buffer, work_baseband_buffer);
		//}

		/* 1.536MHz complex<int16_t>[1024], [-32768, 32512]
		 * -> 3rd order CIC: -0.1dB @ 0.028fs, -1dB @ 0.088fs, -60dB @ 0.468fs
		 *                   -0.1dB @ 43kHz,   -1dB @ 136kHz,  -60dB @ 723kHz
		 * -> gain of 8
		 * -> decimation by 2
		 * -> 768kHz complex<int16_t>[512], [-8192, 8128] */
		auto cic_1_out = cic_1.execute(stage_0_out, work_baseband_buffer);
		if( decimation_factor == DecimationFactor::By4 ) {
			return cic_1_out;
		}

		/* 768kHz complex<int16_t>[512], [-32768, 32512]
		 * -> 3rd order CIC decimation by 2, gain of 1
		 * -> 384kHz complex<int16_t>[256], [-32768, 32512] */
		auto cic_2_out = cic_2.execute(cic_1_out, work_baseband_buffer);
		if( decimation_factor == DecimationFactor::By8 ) {
			return cic_2_out;
		}

		/* 384kHz complex<int16_t>[256], [-32768, 32512]
		 * -> 3rd order CIC decimation by 2, gain of 1
		 * -> 192kHz complex<int16_t>[128], [-32768, 32512] */
		auto cic_3_out = cic_3.execute(cic_2_out, work_baseband_buffer);
		if( decimation_factor == DecimationFactor::By16 ) {
			return cic_3_out;
		}

		/* 192kHz complex<int16_t>[128], [-32768, 32512]
		 * -> 3rd order CIC decimation by 2, gain of 1
		 * -> 96kHz complex<int16_t>[64], [-32768, 32512] */
		auto cic_4_out = cic_4.execute(cic_3_out, work_baseband_buffer);

		return cic_4_out;
	}
};

static volatile bool channel_spectrum_request_update { false };
static std::array<complex16_t, 256> channel_spectrum;
static uint32_t channel_spectrum_sampling_rate { 0 };
static uint32_t channel_filter_pass_frequency { 0 };
static uint32_t channel_filter_stop_frequency { 0 };

class BasebandProcessor {
public:
	virtual ~BasebandProcessor() = default;

	virtual void execute(buffer_c8_t buffer) = 0;

protected:
	BlockDecimator<256> channel_spectrum_decimator { 4 };

	ChannelStatsCollector channel_stats;
	ChannelStatisticsMessage channel_stats_message;

	void feed_channel_stats(const buffer_c16_t channel) {
		channel_stats.feed(
			channel,
			[this](const ChannelStatistics statistics) {
				this->post_channel_stats_message(statistics);
			}
		);
	}

	void post_channel_stats_message(const ChannelStatistics statistics) {
		if( channel_stats_message.is_free() ) {
			channel_stats_message.statistics = statistics;
			shared_memory.application_queue.push(&channel_stats_message);
		}
	}

	void feed_channel_spectrum(const buffer_c16_t channel) {
		channel_spectrum_decimator.feed(
			channel,
			[this](const buffer_c16_t data) {
				this->post_channel_spectrum_message(data);
			}
		);
	}

	void post_channel_spectrum_message(const buffer_c16_t data) {
		if( !channel_spectrum_request_update ) {
			channel_spectrum_request_update = true;
			std::copy(&data.p[0], &data.p[data.count], channel_spectrum.begin());
			channel_spectrum_sampling_rate = data.sampling_rate;
			events_flag(EVT_MASK_SPECTRUM);
		}
	}

	AudioStatsCollector audio_stats;
	AudioStatisticsMessage audio_stats_message;

	void feed_audio_stats(const buffer_s16_t audio) {
		audio_stats.feed(
			audio,
			[this](const AudioStatistics statistics) {
				this->post_audio_stats_message(statistics);
			}
		);
	}

	void post_audio_stats_message(const AudioStatistics statistics) {
		if( audio_stats_message.is_free() ) {
			audio_stats_message.statistics = statistics;
			shared_memory.application_queue.push(&audio_stats_message);
		}
	}

	void fill_audio_buffer(const buffer_s16_t audio) {
		auto audio_buffer = audio::dma::tx_empty_buffer();;
		for(size_t i=0; i<audio_buffer.count; i++) {
			audio_buffer.p[i].left = audio_buffer.p[i].right = audio.p[i];
		}

		i2s::i2s0::tx_unmute();
	}

	void mute_audio() {
		// TODO: Feed audio stats? What if baseband never produces audio?
		// TODO: How should audio stats behave if I *sometimes* mute audio?
		i2s::i2s0::tx_mute();
	}
};

class NarrowbandAMAudio : public BasebandProcessor {
public:
	void execute(buffer_c8_t buffer) override {
		auto decimator_out = decimator.execute(buffer);

		const buffer_c16_t work_baseband_buffer {
			(complex16_t*)decimator_out.p,
			sizeof(*decimator_out.p) * decimator_out.count
		};

		/* 96kHz complex<int16_t>[64]
		 * -> FIR filter, <?kHz (0.???fs) pass, gain 1.0
		 * -> 48kHz int16_t[32] */
		auto channel = channel_filter.execute(decimator_out, work_baseband_buffer);
		channel_filter_pass_frequency = decimator_out.sampling_rate * channel_filter_taps.pass_frequency_normalized;
		channel_filter_stop_frequency = decimator_out.sampling_rate * channel_filter_taps.stop_frequency_normalized;

		// TODO: Feed channel_stats post-decimation data?
		feed_channel_stats(channel);
		feed_channel_spectrum(channel);

		const buffer_s16_t work_audio_buffer {
			(int16_t*)decimator_out.p,
			sizeof(*decimator_out.p) * decimator_out.count
		};

		/* 48kHz complex<int16_t>[32]
		 * -> AM demodulation
		 * -> 48kHz int16_t[32] */
		auto audio = demod.execute(channel, work_audio_buffer);

		audio_hpf.execute(audio);
		feed_audio_stats(audio);
		fill_audio_buffer(audio);
	}

private:
	ChannelDecimator decimator { ChannelDecimator::DecimationFactor::By32 };
	const fir_taps_real<64>& channel_filter_taps = taps_64_lp_031_070_tfilter;
	dsp::decimate::FIRAndDecimateBy2Complex<64> channel_filter { channel_filter_taps.taps };
	dsp::demodulate::AM demod;
	IIRBiquadFilter audio_hpf {
		{  0.93346032f, -1.86687724f,  0.93346032f },
		{  1.0f       , -1.97730264f,  0.97773668f }
	};
};

class NarrowbandFMAudio : public BasebandProcessor {
public:
	void execute(buffer_c8_t buffer) override {
		/* Called every 2048/3072000 second -- 1500Hz. */

		auto decimator_out = decimator.execute(buffer);

		const buffer_c16_t work_baseband_buffer {
			(complex16_t*)decimator_out.p,
			sizeof(*decimator_out.p) * decimator_out.count
		};

		/* 96kHz complex<int16_t>[64]
		 * -> FIR filter, <6kHz (0.063fs) pass, gain 1.0
		 * -> 48kHz int16_t[32] */
		auto channel = channel_filter.execute(decimator_out, work_baseband_buffer);
		channel_filter_pass_frequency = decimator_out.sampling_rate * channel_filter_taps.pass_frequency_normalized;
		channel_filter_stop_frequency = decimator_out.sampling_rate * channel_filter_taps.stop_frequency_normalized;

		// TODO: Feed channel_stats post-decimation data?
		feed_channel_stats(channel);
		feed_channel_spectrum(channel);

		const buffer_s16_t work_audio_buffer {
			(int16_t*)decimator_out.p,
			sizeof(*decimator_out.p) * decimator_out.count
		};

		/* 48kHz complex<int16_t>[32]
		 * -> FM demodulation
		 * -> 48kHz int16_t[32] */
		auto audio = demod.execute(channel, work_audio_buffer);

		audio_hpf.execute(audio);
		feed_audio_stats(audio);
		fill_audio_buffer(audio);
	}

private:
	ChannelDecimator decimator { ChannelDecimator::DecimationFactor::By32 };
	const fir_taps_real<64>& channel_filter_taps = taps_64_lp_042_078_tfilter;
	dsp::decimate::FIRAndDecimateBy2Complex<64> channel_filter { channel_filter_taps.taps };
	dsp::demodulate::FM demod { 48000, 7500 };

	IIRBiquadFilter audio_hpf {
		{  0.93346032f, -1.86687724f,  0.93346032f },
		{  1.0f       , -1.97730264f,  0.97773668f }
	};
};

class WidebandFMAudio : public BasebandProcessor {
public:
	void execute(buffer_c8_t buffer) override {
		auto decimator_out = decimator.execute(buffer);

		const buffer_s16_t work_audio_buffer {
			(int16_t*)decimator_out.p,
			sizeof(*decimator_out.p) * decimator_out.count
		};

		auto channel = decimator_out;

		// TODO: Feed channel_stats post-decimation data?
		feed_channel_stats(channel);
		//feed_channel_spectrum(channel);

		/* 768kHz complex<int16_t>[512]
		 * -> FM demodulation
		 * -> 768kHz int16_t[512] */
		/* TODO: To improve adjacent channel rejection, implement complex channel filter:
		 *		pass < +/- 100kHz, stop > +/- 200kHz
		 */

		auto audio_oversampled = demod.execute(decimator_out, work_audio_buffer);

		/* 768kHz int16_t[512]
		 * -> 4th order CIC decimation by 2, gain of 1
		 * -> 384kHz int16_t[256] */
		auto audio_8fs = audio_dec_1.execute(audio_oversampled, work_audio_buffer);

		/* 384kHz int16_t[256]
		 * -> 4th order CIC decimation by 2, gain of 1
		 * -> 192kHz int16_t[128] */
		auto audio_4fs = audio_dec_2.execute(audio_8fs, work_audio_buffer);

		/* 192kHz int16_t[128]
		 * -> 4th order CIC decimation by 2, gain of 1
		 * -> 96kHz int16_t[64] */
		auto audio_2fs = audio_dec_3.execute(audio_4fs, work_audio_buffer);

		/* 96kHz int16_t[64]
		 * -> FIR filter, <15kHz (0.156fs) pass, >19kHz (0.198fs) stop, gain of 1
		 * -> 48kHz int16_t[32] */
		auto audio = audio_filter.execute(audio_2fs, work_audio_buffer);

		/* -> 48kHz int16_t[32] */
		audio_hpf.execute(audio);
		feed_audio_stats(audio);
		fill_audio_buffer(audio);
	}

private:
	ChannelDecimator decimator { ChannelDecimator::DecimationFactor::By4 };

	//dsp::decimate::FIRAndDecimateBy2Complex<64> channel_filter { taps_64_lp_031_070_tfilter };
	dsp::demodulate::FM demod { 768000, 75000 };
	dsp::decimate::DecimateBy2CIC4Real audio_dec_1;
	dsp::decimate::DecimateBy2CIC4Real audio_dec_2;
	dsp::decimate::DecimateBy2CIC4Real audio_dec_3;
	const fir_taps_real<64>& audio_filter_taps = taps_64_lp_156_198;
	dsp::decimate::FIR64AndDecimateBy2Real audio_filter { audio_filter_taps.taps };

	IIRBiquadFilter audio_hpf {
		{  0.93346032f, -1.86687724f,  0.93346032f },
		{  1.0f       , -1.97730264f,  0.97773668f }
	};
};

class FSKProcessor : public BasebandProcessor {
public:
	FSKProcessor(
		MessageHandlerMap& message_handlers
	) : message_handlers(message_handlers)
	{
		message_handlers[Message::ID::FSKConfiguration] = [this](const Message* const p) {
			auto m = reinterpret_cast<const FSKConfigurationMessage*>(p);
			this->configure(m->configuration);
		};
	}

	~FSKProcessor() {
		message_handlers[Message::ID::FSKConfiguration] = nullptr;
	}

	void configure(const FSKConfiguration new_configuration) {
		clock_recovery.configure(new_configuration.symbol_rate, 76800);
		access_code_correlator.configure(
			new_configuration.access_code,
			new_configuration.access_code_length,
			new_configuration.access_code_tolerance
		);
		packet_builder.configure(new_configuration.packet_length);
	}

	void execute(buffer_c8_t buffer) override {
		/* 2.4576MHz, 2048 samples */

		auto decimator_out = decimator.execute(buffer);

		/* 153.6kHz, 128 samples */

		const buffer_c16_t work_baseband_buffer {
			(complex16_t*)decimator_out.p,
			decimator_out.count
		};

		/* 153.6kHz complex<int16_t>[128]
		 * -> FIR filter, <?kHz (?fs) pass, gain 1.0
		 * -> 76.8kHz int16_t[64] */
		auto channel = channel_filter.execute(decimator_out, work_baseband_buffer);
		channel_filter_pass_frequency = decimator_out.sampling_rate * channel_filter_taps.pass_frequency_normalized;
		channel_filter_stop_frequency = decimator_out.sampling_rate * channel_filter_taps.stop_frequency_normalized;

		/* 76.8kHz, 64 samples */
		feed_channel_stats(channel);
		feed_channel_spectrum(channel);

		const auto symbol_handler_fn = [this](const float value) {
			const uint_fast8_t symbol = (value >= 0.0f) ? 1 : 0;
			const bool access_code_found = this->access_code_correlator.execute(symbol);
			this->consume_symbol(symbol, access_code_found);
		};

		// 76.8k

		const buffer_s16_t work_demod_buffer {
			(int16_t*)decimator_out.p,
			decimator_out.count * sizeof(*decimator_out.p) / sizeof(int16_t)
		};

		auto demodulated = demod.execute(channel, work_demod_buffer);

		mute_audio();

		for(size_t i=0; i<demodulated.count; i++) {
			clock_recovery.execute(demodulated.p[i], symbol_handler_fn);
		}
	}

private:
	ChannelDecimator decimator { ChannelDecimator::DecimationFactor::By16 };
	const fir_taps_real<64>& channel_filter_taps = taps_64_lp_031_070_tfilter;
	dsp::decimate::FIRAndDecimateBy2Complex<64> channel_filter { channel_filter_taps.taps };
	dsp::demodulate::FM demod { 76800, 9600 * 2 };

	ClockRecovery clock_recovery;
	AccessCodeCorrelator access_code_correlator;
	PacketBuilder packet_builder;

	FSKPacketMessage message;
	MessageHandlerMap& message_handlers;

	void consume_symbol(
		const uint_fast8_t symbol,
		const bool access_code_found
	) {
		const auto payload_handler_fn = [this](
			const std::bitset<256>& payload,
			const size_t bits_received
		) {
			this->payload_handler(payload, bits_received);
		};

		packet_builder.execute(
			symbol,
			access_code_found,
			payload_handler_fn
		);
	}

	void payload_handler(
		const std::bitset<256>& payload,
		const size_t bits_received
	) {
		if( message.is_free() ) {
			message.packet.payload = payload;
			message.packet.bits_received = bits_received;
			shared_memory.application_queue.push(&message);
		}
	}
};

static BasebandProcessor* baseband_processor { nullptr };
static BasebandConfiguration baseband_configuration;

static WORKING_AREA(baseband_thread_wa, 8192);
static __attribute__((noreturn)) msg_t baseband_fn(void *arg) {
	(void)arg;
	chRegSetThreadName("baseband");

	BasebandStatsCollector stats;
	BasebandStatisticsMessage message;

	while(true) {
		// TODO: Place correct sampling rate into buffer returned here:
		const auto buffer_tmp = baseband::dma::wait_for_rx_buffer();
		const buffer_c8_t buffer {
			buffer_tmp.p, buffer_tmp.count, baseband_configuration.sampling_rate
		};

		if( baseband_processor ) {
			baseband_processor->execute(buffer);
		}

		stats.process(buffer,
			[&message](const BasebandStatistics statistics) {
				if( message.is_free() ) {
					message.statistics = statistics;
					shared_memory.application_queue.push(&message);
				}
			}
		);
	}
}

static WORKING_AREA(rssi_thread_wa, 128);
static __attribute__((noreturn)) msg_t rssi_fn(void *arg) {
	(void)arg;
	chRegSetThreadName("rssi");

	RSSIStatisticsCollector stats;
	RSSIStatisticsMessage message;

	while(true) {
		// TODO: Place correct sampling rate into buffer returned here:
		const auto buffer_tmp = rf::rssi::dma::wait_for_buffer();
		const rf::rssi::buffer_t buffer {
			buffer_tmp.p, buffer_tmp.count, 400000
		};

		stats.process(
			buffer,
			[&message](const RSSIStatistics statistics) {
				if( message.is_free() ) {
					message.statistics = statistics;
					shared_memory.application_queue.push(&message);
				}
			}
		);
	}
}

extern "C" {

void __late_init(void) {
	/* After this call, scheduler, systick, heap, etc. are available. */
	/* By doing chSysInit() here, it runs before C++ constructors, which may
	 * require the heap.
	 */
	chSysInit();
}

}

static void init() {
	i2s::i2s0::configure(
		audio::i2s0_config_tx,
		audio::i2s0_config_rx,
		audio::i2s0_config_dma
	);

	audio::dma::init();
	audio::dma::configure();
	audio::dma::enable();

	i2s::i2s0::tx_start();
	i2s::i2s0::rx_start();

	LPC_CREG->DMAMUX = portapack::gpdma_mux;
	gpdma::controller.enable();
	nvicEnableVector(DMA_IRQn, CORTEX_PRIORITY_MASK(LPC_DMA_IRQ_PRIORITY));

	baseband::dma::init();

	rf::rssi::init();
	touch::dma::init();

	chThdCreateStatic(baseband_thread_wa, sizeof(baseband_thread_wa),
		baseband_thread_priority, baseband_fn,
		nullptr
	);

	chThdCreateStatic(rssi_thread_wa, sizeof(rssi_thread_wa),
		rssi_thread_priority, rssi_fn,
		nullptr
	);
}

static inline float magnitude_squared(const std::complex<float> c) {
	const auto r = c.real();
	const auto r2 = r * r;
	const auto i = c.imag();
	const auto i2 = i * i;
	return r2 + i2;
}

class EventDispatcher {
public:
	MessageHandlerMap& message_handlers() {
		return message_map;
	}

	eventmask_t wait() {
		return chEvtWaitAny(ALL_EVENTS);
	}

	void dispatch(const eventmask_t events) {
		if( events & EVT_MASK_BASEBAND ) {
			handle_baseband_queue();
		}

		if( events & EVT_MASK_SPECTRUM ) {
			handle_spectrum();
		}
	}

private:
	MessageHandlerMap message_map;

	ChannelSpectrumMessage spectrum_message;
	std::array<uint8_t, 256> spectrum_db;

	void handle_baseband_queue() {
		while( !shared_memory.baseband_queue.is_empty() ) {
			auto message = shared_memory.baseband_queue.pop();

			auto& fn = message_map[message->id];
			if( fn ) {
				fn(message);
			}

			message->state = Message::State::Free;
		}
	}

	void handle_spectrum() {
		if( channel_spectrum_request_update ) {
			/* Decimated buffer is full. Compute spectrum. */
			std::array<std::complex<float>, 256> samples_swapped;
			fft_swap(channel_spectrum, samples_swapped);
			channel_spectrum_request_update = false;
			fft_c_preswapped(samples_swapped);
			if( spectrum_message.is_free() ) {
				for(size_t i=0; i<spectrum_db.size(); i++) {
					const auto mag2 = magnitude_squared(samples_swapped[i]);
					const float db = complex16_mag_squared_to_dbv_norm(mag2);
					constexpr float mag_scale = 5.0f;
					const unsigned int v = (db * mag_scale) + 255.0f;
					spectrum_db[i] = std::max(0U, std::min(255U, v));
				}

				/* TODO: Rename .db -> .magnitude, or something more (less!) accurate. */
				spectrum_message.spectrum.db = &spectrum_db;
				spectrum_message.spectrum.db_count = spectrum_db.size();
				spectrum_message.spectrum.sampling_rate = channel_spectrum_sampling_rate;
				spectrum_message.spectrum.channel_filter_pass_frequency = channel_filter_pass_frequency;
				spectrum_message.spectrum.channel_filter_stop_frequency = channel_filter_stop_frequency;
				shared_memory.application_queue.push(&spectrum_message);
			}
		}
	}
};

static void m0apptxevent_interrupt_enable() {
	nvicEnableVector(M0CORE_IRQn, CORTEX_PRIORITY_MASK(LPC43XX_M0APPTXEVENT_IRQ_PRIORITY));
}

extern "C" {

CH_IRQ_HANDLER(MAPP_IRQHandler) {
	CH_IRQ_PROLOGUE();

	chSysLockFromIsr();
	events_flag_isr(EVT_MASK_BASEBAND);
	chSysUnlockFromIsr();

	creg::m0apptxevent::clear();

	CH_IRQ_EPILOGUE();
}

}

//#define TEST_DSP 1

#if defined(TEST_DSP)
#include "test_dsp.h"
#endif

static constexpr auto direction = baseband::Direction::Receive;

int main(void) {

#if defined(TEST_DSP)
	static TestResultsMessage test_results_message;
	test_results_message.results = test_dsp();
	application_queue.push(&test_results_message);
	while(1);
#else

	init();

	events_initialize(chThdSelf());
	m0apptxevent_interrupt_enable();

	EventDispatcher event_dispatcher;
	auto& message_handlers = event_dispatcher.message_handlers();

	message_handlers[Message::ID::BasebandConfiguration] = [&message_handlers](const Message* const p) {
		auto message = reinterpret_cast<const BasebandConfigurationMessage*>(p);
		if( message->configuration.mode != baseband_configuration.mode ) {

			// TODO: Timing problem around disabling DMA and nulling and deleting old processor
			auto old_p = baseband_processor;
			baseband_processor = nullptr;
			delete old_p;

			switch(message->configuration.mode) {
			case 0:
				baseband_processor = new NarrowbandAMAudio();
				break;

			case 1:
				baseband_processor = new NarrowbandFMAudio();
				break;

			case 2:
				baseband_processor = new WidebandFMAudio();
				break;

			case 3:
				baseband_processor = new FSKProcessor(message_handlers);
				break;

			default:
				break;
			}

			if( baseband_processor ) {
				if( direction == baseband::Direction::Receive ) {
					rf::rssi::start();
				}
				baseband::dma::enable(direction);
			} else {
				baseband::dma::disable();
				rf::rssi::stop();
			}
		}

		baseband_configuration = message->configuration;
	};

	/* TODO: Ensure DMAs are configured to point at first LLI in chain. */

	if( direction == baseband::Direction::Receive ) {
		rf::rssi::dma::allocate(4, 400);
	}

	touch::dma::allocate();
	touch::dma::enable();

	const auto baseband_buffer =
		new std::array<baseband::sample_t, 8192>();
	baseband::dma::configure(
		baseband_buffer->data(),
		direction
	);
	//baseband::dma::allocate(4, 2048);

	while(true) {
		const auto events = event_dispatcher.wait();
		event_dispatcher.dispatch(events);
	}
#endif
	return 0;
}
