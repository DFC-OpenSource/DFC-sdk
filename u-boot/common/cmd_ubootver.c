/*
 * Copyright 2000-2009
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include <version.h>
#include <linux/compiler.h>
#ifdef CONFIG_SYS_COREBOOT
#include <asm/arch/sysinfo.h>
#endif

#ifdef CONFIG_ISSD
const char *local_version = BOOTARG_BINARY_VER;

static int do_version(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	printf("u-boot version: %s\n", (local_version+9));
	return 0;
}

U_BOOT_CMD(
	ubootver,	1,		1,	do_version,
	"print iNIC card u-boot version",
	""
);
#endif
