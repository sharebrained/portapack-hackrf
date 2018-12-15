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

#include "portapack.hpp"
#include "portapack_hal.hpp"
#include "portapack_dma.hpp"
#include "portapack_cpld_data.hpp"
#include "portapack_persistent_memory.hpp"

#include "hackrf_hal.hpp"
#include "hackrf_gpio.hpp"
using namespace hackrf::one;

#include "clock_manager.hpp"

#include "backlight.hpp"
#include "touch_adc.hpp"
#include "audio.hpp"

#include "wm8731.hpp"
using wolfson::wm8731::WM8731;

#include "ak4951.hpp"
using asahi_kasei::ak4951::AK4951;

#include "cpld_update.hpp"

#include "optional.hpp"

namespace portapack {

portapack::IO io {
	portapack::gpio_dir,
	portapack::gpio_lcd_rdx,
	portapack::gpio_lcd_wrx,
	portapack::gpio_io_stbx,
	portapack::gpio_addr,
	portapack::gpio_lcd_te,
	portapack::gpio_unused,
};

portapack::BacklightCAT4004 backlight_cat4004;
portapack::BacklightOnOff   backlight_on_off;

lcd::ILI9341 display;

I2C i2c0(&I2CD0);
SPI ssp1(&SPID2);

si5351::Si5351 clock_generator {
	i2c0, hackrf::one::si5351_i2c_address
};

ClockManager clock_manager {
	i2c0, clock_generator
};

WM8731 audio_codec_wm8731 { i2c0, 0x1a };
AK4951 audio_codec_ak4951 { i2c0, 0x12 };

ReceiverModel receiver_model;

TemperatureLogger temperature_logger;

enum class PortaPackModel {
	R1_20150901,
	R2_20170522,
};

static PortaPackModel portapack_model() {
	static Optional<PortaPackModel> model;

	if( !model.is_valid() ) {
		if( audio_codec_wm8731.detected() ) {
			model = PortaPackModel::R1_20150901;
		} else {
			model = PortaPackModel::R2_20170522;
		}
	}

	return model.value();
}

static audio::Codec* portapack_audio_codec() {
	return (portapack_model() == PortaPackModel::R2_20170522)
		? static_cast<audio::Codec*>(&audio_codec_ak4951)
		: static_cast<audio::Codec*>(&audio_codec_wm8731)
		;
}

static const portapack::cpld::Config& portapack_cpld_config() {
	return (portapack_model() == PortaPackModel::R2_20170522)
		? portapack::cpld::rev_20170522::config
		: portapack::cpld::rev_20150901::config
		;
}

Backlight* backlight() {
	return (portapack_model() == PortaPackModel::R2_20170522)
		? static_cast<portapack::Backlight*>(&backlight_cat4004)
		: static_cast<portapack::Backlight*>(&backlight_on_off);
}

static void shutdown_base() {
	clock_manager.shutdown();

	chSysDisable();

	systick_stop();

	peripherals_reset();
}

bool init() {
	clock_manager.init_peripherals();


	if( !portapack::cpld::update_if_necessary(portapack_cpld_config()) ) {
		shutdown_base();
		return false;
	}

	if( !hackrf::cpld::load_sram() ) {
		chSysHalt();
	}

	portapack::io.init();

	clock_manager.init_clock_generator();
	clock_manager.set_reference_ppb(persistent_memory::correction_ppb());

	audio::init(portapack_audio_codec());
	
	clock_manager.enable_first_if_clock();
	clock_manager.enable_second_if_clock();
	clock_manager.enable_codec_clocks();
	radio::init();

	touch::adc::init();

	LPC_CREG->DMAMUX = portapack::gpdma_mux;
	gpdma::controller.enable();

	return true;
}

void shutdown() {
	gpdma::controller.disable();

	backlight()->off();
	display.shutdown();
	
	radio::disable();
	audio::shutdown();

	hackrf::cpld::init_from_eeprom();

	shutdown_base();
}

} /* namespace portapack */
