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

#ifndef __PINS_H__
#define __PINS_H__

#include "gpio.hpp"

namespace lpc43xx {

enum Pins {
	P0_0, P0_1,
	P1_0, P1_1, P1_2, P1_3, P1_4, P1_5, P1_6, P1_7, P1_8, P1_9, P1_10, P1_11, P1_12, P1_13, P1_14, P1_15, P1_16, P1_17, P1_18, P1_19, P1_20,
	P2_0, P2_1, P2_2, P2_3, P2_4, P2_5, P2_6, P2_7, P2_8, P2_9, P2_10, P2_11, P2_12, P2_13,
	P3_0, P3_1, P3_2,
	P4_0, P4_1, P4_2, P4_3, P4_4, P4_5, P4_6, P4_7, P4_8, P4_9, P4_10,
	P5_0, P5_1, P5_2, P5_3, P5_4, P5_5, P5_6, P5_7,
	P6_0, P6_1, P6_2, P6_3, P6_4, P6_5, P6_6, P6_7, P6_8, P6_9, P6_10, P6_11, P6_12,
	P7_0, P7_1, P7_2, P7_3, P7_4, P7_5, P7_6, P7_7,
	P9_5, P9_6,
	PF_4,
	CLK0, CLK2,
};

constexpr Pin pins[] {
	[P0_0]  = {  0,  0},
	[P0_1]  = {  0,  1},
	[P1_0]  = {  1,  0},
	[P1_1]  = {  1,  1},
	[P1_2]  = {  1,  2},
	[P1_3]  = {  1,  3},
	[P1_4]  = {  1,  4},
	[P1_5]  = {  1,  5},
	[P1_6]  = {  1,  6},
	[P1_7]  = {  1,  7},
	[P1_8]  = {  1,  8},
	[P1_9]  = {  1,  9},
	[P1_10] = {  1, 10},
	[P1_11] = {  1, 11},
	[P1_12] = {  1, 12},
	[P1_13] = {  1, 13},
	[P1_14] = {  1, 14},
	[P1_15] = {  1, 15},
	[P1_16] = {  1, 16},
	[P1_17] = {  1, 17},
	[P1_18] = {  1, 18},
	[P1_19] = {  1, 19},
	[P1_20] = {  1, 20},
	[P2_0]  = {  2,  0},
	[P2_1]  = {  2,  1},
	[P2_2]  = {  2,  2},
	[P2_3]  = {  2,  3},
	[P2_4]  = {  2,  4},
	[P2_5]  = {  2,  5},
	[P2_6]  = {  2,  6},
	[P2_7]  = {  2,  7},
	[P2_8]  = {  2,  8},
	[P2_9]  = {  2,  9},
	[P2_10] = {  2, 10},
	[P2_11] = {  2, 11},
	[P2_12] = {  2, 12},
	[P2_13] = {  2, 13},
	[P3_0]  = {  3,  0},
	[P3_1]  = {  3,  1},
	[P3_2]  = {  3,  2},
	[P4_0]  = {  4,  0},
	[P4_1]  = {  4,  1},
	[P4_2]  = {  4,  2},
	[P4_3]  = {  4,  3},
	[P4_4]  = {  4,  4},
	[P4_5]  = {  4,  5},
	[P4_6]  = {  4,  6},
	[P4_7]  = {  4,  7},
	[P4_8]  = {  4,  8},
	[P4_9]  = {  4,  9},
	[P4_10] = {  4, 10},
	[P5_0]  = {  5,  0},
	[P5_1]  = {  5,  1},
	[P5_2]  = {  5,  2},
	[P5_3]  = {  5,  3},
	[P5_4]  = {  5,  4},
	[P5_5]  = {  5,  5},
	[P5_6]  = {  5,  6},
	[P5_7]  = {  5,  7},
	[P6_0]  = {  6,  0},
	[P6_1]  = {  6,  1},
	[P6_2]  = {  6,  2},
	[P6_3]  = {  6,  3},
	[P6_4]  = {  6,  4},
	[P6_5]  = {  6,  5},
	[P6_6]  = {  6,  6},
	[P6_7]  = {  6,  7},
	[P6_8]  = {  6,  8},
	[P6_9]  = {  6,  9},
	[P6_10] = {  6, 10},
	[P6_11] = {  6, 11},
	[P6_12] = {  6, 12},
	[P7_0]  = {  7,  0},
	[P7_1]  = {  7,  1},
	[P7_2]  = {  7,  2},
	[P7_3]  = {  7,  3},
	[P7_4]  = {  7,  4},
	[P7_5]  = {  7,  5},
	[P7_6]  = {  7,  6},
	[P7_7]  = {  7,  7},
	[P9_5]  = {  9,  5},
	[P9_6]  = {  9,  6},
	[PF_4]  = { 15,  4},
	[CLK0]  = { 24,  0},
	[CLK2]  = { 24,  2},
};

enum GPIOs {
	GPIO0_0, GPIO0_1, GPIO0_2, GPIO0_3, GPIO0_4, GPIO0_5, /*GPIO0_6,*/ GPIO0_7, GPIO0_8, GPIO0_9, GPIO0_10, GPIO0_11, GPIO0_12, GPIO0_13, GPIO0_14, GPIO0_15,
	GPIO1_0, GPIO1_1, GPIO1_2, GPIO1_3, GPIO1_4, GPIO1_5, GPIO1_6, GPIO1_7, GPIO1_8, GPIO1_9, GPIO1_10, GPIO1_11, GPIO1_12, GPIO1_13, /*GPIO1_14, GPIO1_15,*/
	GPIO2_0, GPIO2_1, GPIO2_2, GPIO2_3, GPIO2_4, GPIO2_5, GPIO2_6, GPIO2_7, GPIO2_8, GPIO2_9, GPIO2_10, GPIO2_11, GPIO2_12, GPIO2_13, GPIO2_14, GPIO2_15,
	GPIO3_0, GPIO3_1, GPIO3_2, GPIO3_3, GPIO3_4, GPIO3_5, GPIO3_6, GPIO3_7, GPIO3_8, GPIO3_9, GPIO3_10, GPIO3_11, GPIO3_12, GPIO3_13, GPIO3_14, GPIO3_15,
	GPIO4_11,
	GPIO5_0, GPIO5_1, GPIO5_2, GPIO5_3, GPIO5_4, GPIO5_5, GPIO5_6, GPIO5_7, GPIO5_8, GPIO5_9, /*GPIO5_10, GPIO5_11,*/ GPIO5_12, GPIO5_13, GPIO5_14, GPIO5_15, GPIO5_16, GPIO5_18,
};

constexpr GPIO gpio[] = {
	[GPIO0_0]  = { pins[P0_0],  0,  0, 0 },
	[GPIO0_1]  = { pins[P0_1],  0,  1, 0 },
	[GPIO0_2]  = { pins[P1_15], 0,  2, 0 },
	[GPIO0_3]  = { pins[P1_16], 0,  3, 0 },
	[GPIO0_4]  = { pins[P1_0],  0,  4, 0 },
	[GPIO0_5]  = { pins[P6_6],  0,  5, 0 },
	//[GPIO0_6]  = { pins[P3_6],  0,  6, 0 },
	[GPIO0_7]  = { pins[P2_7],  0,  7, 0 },
	[GPIO0_8]  = { pins[P1_1],  0,  8, 0 },
	[GPIO0_9]  = { pins[P1_2],  0,  9, 0 },
	[GPIO0_10] = { pins[P1_3],  0, 10, 0 },
	[GPIO0_11] = { pins[P1_4],  0, 11, 0 },
	[GPIO0_12] = { pins[P1_17], 0, 12, 0 },
	[GPIO0_13] = { pins[P1_18], 0, 13, 0 },
	[GPIO0_14] = { pins[P2_10], 0, 14, 0 },
	[GPIO0_15] = { pins[P1_20], 0, 15, 0 },

	[GPIO1_0]  = { pins[P1_7],  1,  0, 0 },
	[GPIO1_1]  = { pins[P1_8],  1,  1, 0 },
	[GPIO1_2]  = { pins[P1_9],  1,  2, 0 },
	[GPIO1_3]  = { pins[P1_10], 1,  3, 0 },
	[GPIO1_4]  = { pins[P1_11], 1,  4, 0 },
	[GPIO1_5]  = { pins[P1_12], 1,  5, 0 },
	[GPIO1_6]  = { pins[P1_13], 1,  6, 0 },
	[GPIO1_7]  = { pins[P1_14], 1,  7, 0 },
	[GPIO1_8]  = { pins[P1_5],  1,  8, 0 },
	[GPIO1_9]  = { pins[P1_6],  1,  9, 0 },
	[GPIO1_10] = { pins[P2_9],  1, 10, 0 },
	[GPIO1_11] = { pins[P2_11], 1, 11, 0 },
	[GPIO1_12] = { pins[P2_12], 1, 12, 0 },
	[GPIO1_13] = { pins[P2_13], 1, 13, 0 },
	//[GPIO1_14] = { pins[P3_4],  1, 14, 0 },
	//[GPIO1_15] = { pins[P3_5],  1, 15, 0 },

	[GPIO2_0]  = { pins[P4_0],  2,  0, 0 },
	[GPIO2_1]  = { pins[P4_1],  2,  1, 0 },
	[GPIO2_2]  = { pins[P4_2],  2,  2, 0 },
	[GPIO2_3]  = { pins[P4_3],  2,  3, 0 },
	[GPIO2_4]  = { pins[P4_4],  2,  4, 0 },
	[GPIO2_5]  = { pins[P4_5],  2,  5, 0 },
	[GPIO2_6]  = { pins[P4_6],  2,  6, 0 },
	[GPIO2_7]  = { pins[P5_7],  2,  7, 0 },
	[GPIO2_8]  = { pins[P6_12], 2,  8, 0 },
	[GPIO2_9]  = { pins[P5_0],  2,  9, 0 },
	[GPIO2_10] = { pins[P5_1],  2, 10, 0 },
	[GPIO2_11] = { pins[P5_2],  2, 11, 0 },
	[GPIO2_12] = { pins[P5_3],  2, 12, 0 },
	[GPIO2_13] = { pins[P5_4],  2, 13, 0 },
	[GPIO2_14] = { pins[P5_5],  2, 14, 0 },
	[GPIO2_15] = { pins[P5_6],  2, 15, 0 },

	[GPIO3_0]  = { pins[P6_1],  3,  0, 0 },
	[GPIO3_1]  = { pins[P6_2],  3,  1, 0 },
	[GPIO3_2]  = { pins[P6_3],  3,  2, 0 },
	[GPIO3_3]  = { pins[P6_4],  3,  3, 0 },
	[GPIO3_4]  = { pins[P6_5],  3,  4, 0 },
	[GPIO3_5]  = { pins[P6_9],  3,  5, 0 },
	[GPIO3_6]  = { pins[P6_10], 3,  6, 0 },
	[GPIO3_7]  = { pins[P6_11], 3,  7, 0 },
	[GPIO3_8]  = { pins[P7_0],  3,  8, 0 },
	[GPIO3_9]  = { pins[P7_1],  3,  9, 0 },
	[GPIO3_10] = { pins[P7_2],  3, 10, 0 },
	[GPIO3_11] = { pins[P7_3],  3, 11, 0 },
	[GPIO3_12] = { pins[P7_4],  3, 12, 0 },
	[GPIO3_13] = { pins[P7_5],  3, 13, 0 },
	[GPIO3_14] = { pins[P7_6],  3, 14, 0 },
	[GPIO3_15] = { pins[P7_7],  3, 15, 0 },

	[GPIO4_11] = { pins[P9_6],  4, 11, 0 },

	[GPIO5_0]  = { pins[P2_0],  5,  0, 4 },
	[GPIO5_1]  = { pins[P2_1],  5,  1, 4 },
	[GPIO5_2]  = { pins[P2_2],  5,  2, 4 },
	[GPIO5_3]  = { pins[P2_3],  5,  3, 4 },
	[GPIO5_4]  = { pins[P2_4],  5,  4, 4 },
	[GPIO5_5]  = { pins[P2_5],  5,  5, 4 },
	[GPIO5_6]  = { pins[P2_6],  5,  6, 4 },
	[GPIO5_7]  = { pins[P2_8],  5,  7, 4 },
	[GPIO5_8]  = { pins[P3_1],  5,  8, 4 },
	[GPIO5_9]  = { pins[P3_2],  5,  9, 4 },
	//[GPIO5_10] = { pins[P3_7],  5, 10, 4 },
	//[GPIO5_11] = { pins[P3_8],  5, 11, 4 },
	[GPIO5_12] = { pins[P4_8],  5, 12, 4 },
	[GPIO5_13] = { pins[P4_9],  5, 13, 4 },
	[GPIO5_14] = { pins[P4_10], 5, 14, 4 },
	[GPIO5_15] = { pins[P6_7],  5, 15, 4 },
	[GPIO5_16] = { pins[P6_8],  5, 16, 4 },
	[GPIO5_18] = { pins[P9_5],  5, 18, 4 },
};

}

#endif/*__PINS_H__*/
