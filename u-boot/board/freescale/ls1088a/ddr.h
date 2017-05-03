/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __DDR_H__
#define __DDR_H__
struct board_specific_parameters {
	u32 n_ranks;
	u32 datarate_mhz_high;
	u32 rank_gb;
	u32 clk_adjust;
	u32 wrlvl_start;
	u32 wrlvl_ctl_2;
	u32 wrlvl_ctl_3;
};

/*
 * These tables contain all valid speeds we want to override with board
 * specific parameters. datarate_mhz_high values need to be in ascending order
 * for each n_ranks group.
 */

static const struct board_specific_parameters udimm0[] = {
	/*
	 * memory controller 0
	 *   num|  hi| rank|  clk| wrlvl |   wrlvl   |  wrlvl
	 * ranks| mhz| GB  |adjst| start |   ctl2    |  ctl3
	 */
#if defined(CONFIG_TARGET_LS1088AQDS)
	{2,  1666, 0, 8,   8, 0x0A0A0C0E, 0x0F10110C,},
	{2,  1900, 0, 4,   7, 0x09090B0D, 0x0E10120B,},
	{2,  2300, 0, 4,   9, 0x0A0C0D11, 0x1214150E,},
	{}
#elif defined(CONFIG_TARGET_LS1088ARDB)
	{2,  1666, 0, 8,   8, 0x090A0B0E, 0x0F10110D,},
	{2,  1900, 0, 4,   7, 0x09090B0D, 0x0E10120B,},
	{2,  2300, 0, 8,   9, 0x0A0C0E11, 0x1214160F,},
#elif defined(CONFIG_EMU)
	{2,  2140, 0, 4,   4, 0x0, 0x0},
	{1,  2140, 0, 4,   4, 0x0, 0x0},
	{}
#endif
};

static const struct board_specific_parameters *udimms[] = {
	udimm0,
};
#endif
