/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2018 Furrtek
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

#include "ui_scanner.hpp"

#include "baseband_api.hpp"
#include "string_format.hpp"
#include "audio.hpp"

using namespace portapack;

namespace ui {

ScannerThread::ScannerThread(
	std::vector<rf::Frequency> frequency_list
) : frequency_list_ {  std::move(frequency_list) }
{
	thread = chThdCreateFromHeap(NULL, 1024, NORMALPRIO + 10, ScannerThread::static_fn, this);
}

ScannerThread::~ScannerThread() {
	if( thread ) {
		chThdTerminate(thread);
		chThdWait(thread);
		thread = nullptr;
	}
}

void ScannerThread::set_scanning(const bool v) {
	_scanning = v;
}

msg_t ScannerThread::static_fn(void* arg) {
	auto obj = static_cast<ScannerThread*>(arg);
	obj->run();
	return 0;
}

void ScannerThread::run() {
	RetuneMessage message { };
	uint32_t frequency_index = 0;
	
	while( !chThdShouldTerminate() ) {
		if (_scanning) {
			// Retune
			receiver_model.set_tuning_frequency(frequency_list_[frequency_index]);
			
			message.range = frequency_list_[frequency_index];
			EventDispatcher::send_message(message);
			
			
			frequency_index++;
			if (frequency_index >= frequency_list_.size())
				frequency_index = 0;
		}
		
		chThdSleepMilliseconds(50);
	}
}

void ScannerView::handle_retune(uint32_t i) {
	text_cycle.set("Scanning: " + to_string_dec_uint(i)); // + "/" + to_string_dec_uint(frequency_list.size()));
}

void ScannerView::focus() {
	field_lna.focus();
}

ScannerView::~ScannerView() {
	audio::output::stop();
	receiver_model.disable();
	baseband::shutdown();
}

ScannerView::ScannerView(
	NavigationView&
)
{
	add_children({
		&labels,
		&field_lna,
		&field_vga,
		&field_rf_amp,
		&field_volume,
		&field_squelch,
		//&record_view,
		&text_cycle,
		//&waterfall,
	});
	
       //read scanner frequency file

	File scanner_file;
	size_t file_position = 0;
	char * line_start;
	std::string description;
	char file_data[13];
	//freqman_entry_type type;
	
	//db.clear();
	
	auto result = scanner_file.open("/scanner-freq.TXT"); // This file has one frequency per line, with mandatory 10 digits (Hz) + CrLF = 12 characters
	frequency_list.push_back(144000000); // In case the file does not contain anything, here is one frequency
	
	while (1) {
		// Read a 10 bytes block from file
		scanner_file.seek(file_position);
		
		memset(file_data, 0, 13);
		auto read_size = scanner_file.read(file_data, 12);
		
		file_position += 12;
		
		// Reset line_start to beginning of buffer
		line_start = file_data;
		
		if (strstr(file_data, "END")) // Last line of file needs to contain "ENDENDENDE"+CrLf
			break;
		
		// Read frequency
		frequency_list.push_back(atol(line_start));
		
	}


	field_squelch.on_change = [this](int32_t v) {
		squelch = v;
	};
	field_squelch.set_value(30);

	field_volume.set_value((receiver_model.headphone_volume() - audio::headphone::volume_range().max).decibel() + 99);
	field_volume.on_change = [this](int32_t v) {
		this->on_headphone_volume_changed(v);
	};
	
	audio::output::start();
	
	audio::output::mute();
	baseband::run_image(portapack::spi_flash::image_tag_nfm_audio);
	receiver_model.set_modulation(ReceiverModel::Mode::NarrowbandFMAudio);
	receiver_model.set_sampling_rate(3072000);
	receiver_model.set_baseband_bandwidth(1750000);
	receiver_model.enable();
	receiver_model.set_squelch_level(0);
	receiver_model.set_nbfm_configuration(0);	// 8k5 <-- changed from 2 to 0 (16k to 8k5)
	audio::output::unmute();
	
	// TODO: Scanning thread here
	scan_thread = std::make_unique<ScannerThread>(frequency_list);
}

void ScannerView::on_statistics_update(const ChannelStatistics& statistics) {
	int32_t max_db = statistics.max_db;
	
	if (timer < 26) // Increased timer from 5 to 25
		timer++;
	
	if (max_db < -squelch) {
		if (timer == 25) { // Increased timer from 5 to 25 to allow for some extra time after transmission finishes
			//audio::output::stop();
			scan_thread->set_scanning(true);
		}
	} else {
		//audio::output::start();
		scan_thread->set_scanning(false);
		timer = 0;
	}
}

void ScannerView::on_headphone_volume_changed(int32_t v) {
	const auto new_volume = volume_t::decibel(v - 99) + audio::headphone::volume_range().max;
	receiver_model.set_headphone_volume(new_volume);
}

} /* namespace ui */
