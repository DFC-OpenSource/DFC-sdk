/*
 * Copyright (C) 2013 - ARM Ltd
 * Author: Marc Zyngier <marc.zyngier@arm.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __ARM_PSCI_H__
#define __ARM_PSCI_H__

#define PSCI_STACK_ALIGN_SIZE		0x1000

/* 1kB for percpu stack */
#define PSCI_PERCPU_STACK_SIZE		0x400

#define PSCI_FN_BASE			0x84000000
#define PSCI_FN_ID(n)			(PSCI_FN_BASE + (n))

/* PSCI v0.1 interface */
#define PSCI_FN_CPU_SUSPEND		PSCI_FN_ID(1)
#define PSCI_FN_CPU_OFF			PSCI_FN_ID(2)
#define PSCI_FN_CPU_ON			PSCI_FN_ID(3)
#define PSCI_FN_MIGRATE			PSCI_FN_ID(5)

/* PSCI v0.2 interface */
#define PSCI_FN_PSCI_VERSION		PSCI_FN_ID(0)
#define PSCI_FN_AFFINITY_INFO		PSCI_FN_ID(4)
#define PSCI_FN_MIGRATE_INFO_TYPE	PSCI_FN_ID(6)
#define PSCI_FN_MIGRATE_INFO_UP_CPU	PSCI_FN_ID(7)
#define PSCI_FN_SYSTEM_OFF		PSCI_FN_ID(8)
#define PSCI_FN_SYSTEM_RESET		PSCI_FN_ID(9)

/* PSCI v1.0 interface */
#define PSCI_FN_PSCI_FEATURES		PSCI_FN_ID(10)
#define PSCI_FN_SYSTEM_SUSPEND		PSCI_FN_ID(14)


/* PSCI features decoding (>=1.0) */
#define PSCI_FEATURES_CPU_SUSPEND_PF_MASK	0x2
#define PSCI_FEATURES_CPU_SUSPEND_OSIM_MASK	0x1

/*
 * Original from Linux kernel: include/uapi/linux/psci.h
 */
/* PSCI v0.2 affinity level state returned by AFFINITY_INFO */
#define PSCI_AFFINITY_LEVEL_ON		0
#define PSCI_AFFINITY_LEVEL_OFF		1
#define PSCI_AFFINITY_LEVEL_ON_PENDING	2

/* PSCI return values (inclusive of all PSCI versions) */
#define PSCI_RET_SUCCESS		0
#define PSCI_RET_NOT_SUPPORTED		(-1)
#define PSCI_RET_INVALID_PARAMS		(-2)
#define PSCI_RET_DENIED			(-3)
#define PSCI_RET_ALREADY_ON             (-4)
#define PSCI_RET_ON_PENDING             (-5)
#define PSCI_RET_INTERNAL_FAILURE       (-6)
#define PSCI_RET_NOT_PRESENT            (-7)
#define PSCI_RET_DISABLED               (-8)

#ifndef __ASSEMBLY__
int psci_update_dt(void *fdt);
void psci_board_init(void);
#endif /* ! __ASSEMBLY__ */

#endif /* __ARM_PSCI_H__ */
