/*
 * Copyright 2015 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __SEC_FIRMWARE_H_
#define __SEC_FIRMWARE_H_

#ifdef CONFIG_FSL_LS_PPA
#include <asm/arch/ppa.h>
#endif

#ifdef CONFIG_ARMV8_PSCI
unsigned int sec_firmware_support_psci_version(void);
#endif
int sec_firmware_validate(void);

#endif /* __SEC_FIRMWARE_H_ */
