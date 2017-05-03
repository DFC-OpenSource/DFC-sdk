/*
 * Copyright 2015 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __FSL_SECURE_BOOT_H
#define __FSL_SECURE_BOOT_H

#ifdef CONFIG_SECURE_BOOT

#ifndef CONFIG_FIT_SIGNATURE
#define CONFIG_CHAIN_OF_TRUST
#endif

#endif

#ifdef CONFIG_CHAIN_OF_TRUST
#define CONFIG_CMD_ESBC_VALIDATE
#define CONFIG_CMD_BLOB
#define CONFIG_CMD_HASH
#define CONFIG_FSL_SEC_MON
#define CONFIG_SHA_HW_ACCEL
#define CONFIG_SHA_PROG_HW_ACCEL
#define CONFIG_RSA
#define CONFIG_RSA_FREESCALE_EXP

#ifndef CONFIG_FSL_CAAM
#define CONFIG_FSL_CAAM
#endif

#ifndef CONFIG_DM
#define CONFIG_DM
#endif

#define CONFIG_KEY_REVOCATION
#ifndef CONFIG_SYS_RAMBOOT
/* The key used for verification of next level images
 * is picked up from an Extension Table which has
 * been verified by the ISBC (Internal Secure boot Code)
 * in boot ROM of the SoC.
 * The feature is only applicable in case of NOR boot and is
 * not applicable in case of RAMBOOT (NAND, SD, SPI).
 * For LS, this feature is available for all device if IE Table
 * is copied to XIP memory
 * Also, for LS, ISBC doesn't verify this table.
 */
#define CONFIG_FSL_ISBC_KEY_EXT

#endif

#ifdef CONFIG_FSL_LAYERSCAPE
/* For LS1043 (ARMv8), ESBC image Address in Header is 64 bit
 * Similiarly for LS2080 and LS2085
 */
#define CONFIG_ESBC_ADDR_64BIT
#endif

#ifdef CONFIG_FSL_LSCH3
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

/* Copying Bootscript and Header to DDR from NOR for LS2 and for rest, from
 * Non-XIP Memory (Nand/SD)*/
#if defined(CONFIG_SYS_RAMBOOT) || defined(CONFIG_FSL_LSCH3)
#define CONFIG_BOOTSCRIPT_COPY_RAM
#endif
/* The address needs to be modified according to NOR and DDR memory map */
#ifdef CONFIG_FSL_LSCH3
#ifdef CONFIG_QSPI_BOOT
#define CONFIG_BS_COPY_QSPI_IP
#define CONFIG_BS_HDR_ADDR_FLASH        0xd00000
#define CONFIG_BS_ADDR_FLASH            0xe00000
#else /* NOR BOOT */
#define CONFIG_BS_HDR_ADDR_FLASH        0x580d00000
#define CONFIG_BS_ADDR_FLASH            0x580e00000
#endif /*ifdef CONFIG_QSPI_BOOT */
#define CONFIG_BS_HDR_ADDR_RAM          0xa0d00000
#define CONFIG_BS_ADDR_RAM              0xa0e00000
#else /* !ifdef CONFIG_FSL_LSCH3 */
#define CONFIG_BS_HDR_ADDR_FLASH	0x600a0000
#define CONFIG_BS_ADDR_FLASH		0x60060000
#define CONFIG_BS_HDR_ADDR_RAM		0xa0060000
#define CONFIG_BS_ADDR_RAM		0xa0060000
#endif

#ifdef CONFIG_BOOTSCRIPT_COPY_RAM
#define CONFIG_BOOTSCRIPT_HDR_ADDR	CONFIG_BS_HDR_ADDR_RAM
#define CONFIG_BS_HDR_SIZE		0x00002000
#define CONFIG_BOOTSCRIPT_ADDR		CONFIG_BS_ADDR_RAM
#define CONFIG_BS_SIZE			0x00001000
#else
#define CONFIG_BOOTSCRIPT_HDR_ADDR	CONFIG_BS_HDR_ADDR_FLASH
/* BS_HDR_SIZE, BOOTSCRIPT_ADDR and BS_SIZE are not required */
#endif

#ifdef CONFIG_FSL_LS_PPA
#ifdef CONFIG_SYS_LS_PPA_FW_IN_XIP
#ifdef CONFIG_LS1043A
#define CONFIG_SYS_LS_PPA_ESBC_ADDR	0x600c0000
#elif defined(CONFIG_FSL_LSCH3)
#define CONFIG_SYS_LS_PPA_ESBC_ADDR     0x580c40000
#endif
#else
#error "No CONFIG_SYS_LS_PPA_FW_IN_xxx defined"
#endif
#endif /* CONFIG_FSL_LS_PPA */

/* Define the key hash here if SRK used for signing PPA image is
 * different from SRK hash put in SFP used for U-Boot.
 * Example
 * #define CONFIG_PPA_KEY_HASH \
 *	"41066b564c6ffcef40ccbc1e0a5d0d519604000c785d97bbefd25e4d288d1c8b"
 */
#define CONFIG_PPA_KEY_HASH		NULL

#include <config_fsl_chain_trust.h>
#endif /* #ifdef CONFIG_CHAIN_OF_TRUST */
#endif
