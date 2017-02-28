/*
 * Copyright 2015 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __FSL_SECURE_BOOT_H
#define __FSL_SECURE_BOOT_H

#ifdef CONFIG_CHAIN_OF_TRUST
#define CONFIG_CMD_ESBC_VALIDATE
#define CONFIG_CMD_BLOB
#define CONFIG_FSL_SEC_MON
#define CONFIG_SHA_PROG_HW_ACCEL
#ifndef CONFIG_DM
#define CONFIG_DM
#endif
#define CONFIG_RSA
#define CONFIG_RSA_FREESCALE_EXP
#ifndef CONFIG_FSL_CAAM
#define CONFIG_FSL_CAAM
#endif

#define CONFIG_KEY_REVOCATION
#ifndef CONFIG_SYS_RAMBOOT
/* The key used for verification of next level images
 * is picked up from an Extension Table which has
 * been verified by the ISBC (Internal Secure boot Code)
 * in boot ROM of the SoC.
 * The feature is only applicable in case of NOR boot and is
 * not applicable in case of RAMBOOT (NAND, SD, SPI).
 */
#ifndef CONFIG_ESBC_HDR_LS
/* Current Key EXT feature not available in LS ESBC Header */
#define CONFIG_FSL_ISBC_KEY_EXT
#endif

#endif

#ifndef CONFIG_FIT_SIGNATURE

#if defined(CONFIG_LS2080A) || defined(CONFIG_LS2085A)
#define CONFIG_EXTRA_ENV \
	"setenv fdt_high 0xa0000000;"	\
	"setenv initrd_high 0xcfffffff;"	\
	"setenv hwconfig \'fsl_ddr:ctlr_intlv=null,bank_intlv=null\';"
#else
#define CONFIG_EXTRA_ENV \
	"setenv fdt_high 0xcfffffff;"	\
	"setenv initrd_high 0xcfffffff;"	\
	"setenv hwconfig \'fsl_ddr:ctlr_intlv=null,bank_intlv=null\';"
#endif

/* Copying Bootscript and Header to DDR from NOR*/
#define CONFIG_BOOTSCRIPT_COPY_RAM
/* The address needs to be modified according to NOR and DDR memory map */
#if defined(CONFIG_LS2080A) || defined(CONFIG_LS2085A)
#define CONFIG_BS_HDR_ADDR_FLASH	0x583920000
#define CONFIG_BS_ADDR_FLASH		0x583900000
#define CONFIG_BS_HDR_ADDR_RAM		0xa3920000
#define CONFIG_BS_ADDR_RAM		0xa3900000
#else
#define CONFIG_BS_HDR_ADDR_FLASH	0x600a0000
#define CONFIG_BS_ADDR_FLASH		0x60060000
#define CONFIG_BS_HDR_ADDR_RAM		0xa0060000
#define CONFIG_BS_ADDR_RAM		0xa0060000
#endif

#define CONFIG_BOOTSCRIPT_HDR_ADDR	CONFIG_BS_HDR_ADDR_RAM
#define CONFIG_BS_HDR_SIZE		0x00002000
#define CONFIG_BOOTSCRIPT_ADDR		CONFIG_BS_ADDR_RAM
#define CONFIG_BS_SIZE			0x00001000

#include <config_fsl_chain_trust.h>
#endif
#endif /* #ifdef CONFIG_CHAIN_OF_TRUST */

#endif
