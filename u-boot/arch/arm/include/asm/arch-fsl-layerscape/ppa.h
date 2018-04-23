/*
 * Copyright 2015 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __FSL_PPA_H_
#define __FSL_PPA_H_

int ppa_init_pre(u64 *);
int ppa_init_entry(void *);
int ppa_init(void *, u32*, u32*);
unsigned long ppa_get_dram_block_size(void);
unsigned int ppa_support_psci_version(void);

#endif
