/*
 * Copyright (C) 2013,2014 - ARM Ltd
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

#include <config.h>
#include <linux/linkage.h>
#include <asm/macro.h>
#include <asm/psci.h>

#define SAVE_SPACE_TARGET_PC_OFFSET	0x0
#define SAVE_SPACE_CONTEXT_ID_OFFSET	0x4

	.pushsection ._secure.text, "ax"

	.arch_extension	sec

	.align	5
	.globl _psci_vectors
_psci_vectors:
	b	default_psci_vector	@ reset
	b	default_psci_vector	@ undef
	b	_smc_psci		@ smc
	b	default_psci_vector	@ pabort
	b	default_psci_vector	@ dabort
	b	default_psci_vector	@ hyp
	b	default_psci_vector	@ irq
	b	psci_fiq_enter		@ fiq

ENTRY(psci_fiq_enter)
	movs	pc, lr
ENDPROC(psci_fiq_enter)
.weak psci_fiq_enter

ENTRY(default_psci_vector)
	movs	pc, lr
ENDPROC(default_psci_vector)
.weak default_psci_vector

ENTRY(psci_cpu_suspend)
ENTRY(psci_cpu_off)
ENTRY(psci_cpu_on)
ENTRY(psci_migrate)
#ifdef CONFIG_ARMV7_PSCI_GTE_1_0
ENTRY(psci_migrate_info_type)
ENTRY(psci_migrate_info_up_cpu)
ENTRY(psci_system_suspend)
#endif
	mov	r0, #PSCI_RET_NOT_SUPPORTED	@ Return -1 (Not Implemented)
	mov	pc, lr
#ifdef CONFIG_ARMV7_PSCI_GTE_1_0
ENDPROC(psci_system_suspend)
ENDPROC(psci_migrate_info_up_cpu)
ENDPROC(psci_migrate_info_type)
#endif
ENDPROC(psci_migrate)
ENDPROC(psci_cpu_on)
ENDPROC(psci_cpu_off)
ENDPROC(psci_cpu_suspend)
.weak psci_cpu_suspend
.weak psci_cpu_off
.weak psci_cpu_on
.weak psci_migrate
#ifdef CONFIG_ARMV7_PSCI_GTE_1_0
.weak psci_migrate_info_type
.weak psci_migrate_info_up_cpu
.weak psci_system_suspend
#endif

_psci_table:
	/* PSCI v0.1 */
	.word	PSCI_FN_CPU_SUSPEND
	.word	psci_cpu_suspend
	.word	PSCI_FN_CPU_OFF
	.word	psci_cpu_off
	.word	PSCI_FN_CPU_ON
	.word	psci_cpu_on_common
	.word	PSCI_FN_MIGRATE
	.word	psci_migrate
#ifdef CONFIG_ARMV7_PSCI_GTE_1_0
	/* PSCI v0.2 */
	.word	PSCI_FN_PSCI_VERSION
	.word	psci_version
	.word	PSCI_FN_AFFINITY_INFO
	.word	psci_affinity_info
	.word	PSCI_FN_MIGRATE_INFO_TYPE
	.word	psci_migrate_info_type
	.word	PSCI_FN_MIGRATE_INFO_UP_CPU
	.word	psci_migrate_info_up_cpu
	.word	PSCI_FN_SYSTEM_OFF
	.word	psci_system_off
	.word	PSCI_FN_SYSTEM_RESET
	.word	psci_system_reset
	/* PSCI v1.0 */
	.word	PSCI_FN_PSCI_FEATURES
	.word	psci_features
	.word	PSCI_FN_SYSTEM_SUSPEND
	.word	psci_system_suspend
#endif
	.word	0
	.word	0

_smc_psci:
	push	{r4-r12,lr}

	@ Switch to secure
	mrc	p15, 0, r7, c1, c1, 0
	bic	r4, r7, #1
	mcr	p15, 0, r4, c1, c1, 0
	isb

	adr	r4, _psci_table
1:	ldr	r5, [r4]		@ Load PSCI function ID
	ldr	r6, [r4, #4]		@ Load target PC
	cmp	r5, #0			@ If reach the end, bail out
	moveq	r0, #PSCI_RET_INVALID_PARAMS	@ Return -2 (Invalid)
	beq	2f
	cmp	r0, r5			@ If not matching, try next entry
	addne	r4, r4, #8
	bne	1b

	blx	r6			@ Execute PSCI function

	@ Switch back to non-secure
2:	mcr	p15, 0, r7, c1, c1, 0

	pop	{r4-r12, lr}
	movs	pc, lr			@ Return to the kernel

@ Requires dense and single-cluster CPU ID space
ENTRY(psci_get_cpu_id)
	mrc	p15, 0, r0, c0, c0, 5	/* read MPIDR */
	and	r0, r0, #0xff		/* return CPU ID in cluster */
	bx	lr
ENDPROC(psci_get_cpu_id)
.weak psci_get_cpu_id

/* Imported from Linux kernel */
LENTRY(v7_flush_dcache_all)
	dmb					@ ensure ordering with previous memory accesses
	mrc	p15, 1, r0, c0, c0, 1		@ read clidr
	ands	r3, r0, #0x7000000		@ extract loc from clidr
	mov	r3, r3, lsr #23			@ left align loc bit field
	beq	finished			@ if loc is 0, then no need to clean
	mov	r10, #0				@ start clean at cache level 0
flush_levels:
	add	r2, r10, r10, lsr #1		@ work out 3x current cache level
	mov	r1, r0, lsr r2			@ extract cache type bits from clidr
	and	r1, r1, #7			@ mask of the bits for current cache only
	cmp	r1, #2				@ see what cache we have at this level
	blt	skip				@ skip if no cache, or just i-cache
	mrs     r9, cpsr			@ make cssr&csidr read atomic
	mcr	p15, 2, r10, c0, c0, 0		@ select current cache level in cssr
	isb					@ isb to sych the new cssr&csidr
	mrc	p15, 1, r1, c0, c0, 0		@ read the new csidr
	msr     cpsr_c, r9
	and	r2, r1, #7			@ extract the length of the cache lines
	add	r2, r2, #4			@ add 4 (line length offset)
	ldr	r4, =0x3ff
	ands	r4, r4, r1, lsr #3		@ find maximum number on the way size
	clz	r5, r4				@ find bit position of way size increment
	ldr	r7, =0x7fff
	ands	r7, r7, r1, lsr #13		@ extract max number of the index size
loop1:
	mov	r9, r7				@ create working copy of max index
loop2:
	orr	r11, r10, r4, lsl r5		@ factor way and cache number into r11
	orr	r11, r11, r9, lsl r2		@ factor index number into r11
	mcr	p15, 0, r11, c7, c14, 2		@ clean & invalidate by set/way
	subs	r9, r9, #1			@ decrement the index
	bge	loop2
	subs	r4, r4, #1			@ decrement the way
	bge	loop1
skip:
	add	r10, r10, #2			@ increment cache number
	cmp	r3, r10
	bgt	flush_levels
finished:
	mov	r10, #0				@ swith back to cache level 0
	mcr	p15, 2, r10, c0, c0, 0		@ select current cache level in cssr
	dsb	st
	isb
	bx	lr
ENDPROC(v7_flush_dcache_all)

ENTRY(psci_disable_smp)
	mrc	p15, 0, r0, c1, c0, 1		@ ACTLR
	bic	r0, r0, #(1 << 6)		@ Clear SMP bit
	mcr	p15, 0, r0, c1, c0, 1		@ ACTLR
	isb
	dsb
	bx	lr
ENDPROC(psci_disable_smp)
.weak psci_disable_smp

ENTRY(psci_enable_smp)
	mrc	p15, 0, r0, c1, c0, 1		@ ACTLR
	orr	r0, r0, #(1 << 6)		@ Set SMP bit
	mcr	p15, 0, r0, c1, c0, 1		@ ACTLR
	isb
	bx	lr
ENDPROC(psci_enable_smp)
.weak psci_enable_smp

@ Expect target CPU in r1, return the target cpu number in r0.
@ If detect an error, please return #PSCI_RET_INVALID_PARAMS.
ENTRY(psci_get_target_cpu_id)
	mov	r0, r1
	bx	lr
ENDPROC(psci_get_target_cpu_id)
.weak psci_get_target_cpu_id

	@ r1 = target CPU
	@ r2 = target PC
	@ r3 = target Conetxt ID
ENTRY(psci_cpu_on_common)
	push	{lr}

	bl	psci_get_target_cpu_id
	cmp	r0, #PSCI_RET_INVALID_PARAMS
	beq	out_psci_cpu_on_common

	@ Update the target CPU ID, translation have been completed.
	mov	r1, r0

	@ Save target PC into stack
	bl	psci_save_target_pc

	@ Save target context into stack
	mov	r0, r1
	bl	psci_save_cpu_context_id

	@ Still pass on:
	@ r1 = target CPU
	@ r2 = target PC
	@ r3 = target Conetxt ID
	bl	psci_cpu_on

out_psci_cpu_on_common:
	pop	{pc}
ENDPROC(psci_cpu_on_common)

ENTRY(psci_cpu_off_common)
	push	{lr}

	mrc	p15, 0, r0, c1, c0, 0		@ SCTLR
	bic	r0, r0, #(1 << 2)		@ Clear C bit
	mcr	p15, 0, r0, c1, c0, 0		@ SCTLR
	isb
	dsb

	bl	v7_flush_dcache_all

	clrex					@ Why???

	bl	psci_disable_smp

	pop	{lr}
	bx	lr
ENDPROC(psci_cpu_off_common)

@ expects CPU ID in r0 and returns stack top in r0
ENTRY(psci_get_cpu_stack_top)
	@ Align psci_text_end minumum page size. Even if page is greater than
	@ 4K, we still can ensure that data is always safe access by the CPU.
	ldr	r5, =psci_text_end		@ end of monitor text
	@ Detect page border
	lsl	r4, r5, #20
	cmp	r4, #0
	beq	out_psci_stack_align
	add	r5, r5, #PSCI_STACK_ALIGN_SIZE
	lsr	r5, r5, #12
	lsl	r5, r5, #12

out_psci_stack_align:
	mov	r4, #PSCI_PERCPU_STACK_SIZE
	add	r0, r0, #1
	mul	r0, r0, r4
	add	r0, r0, r5

	bx	lr
ENDPROC(psci_get_cpu_stack_top)

@ Save space in percpu stack tail.
@ expects CPU ID in r0 and returns save space address in r0.
ENTRY(psci_get_cpu_save_space)
	mov	r10, lr

	bl	psci_get_cpu_stack_top
	sub	r0, r0, #PSCI_PERCPU_STACK_SIZE

	bx	r10
ENDPROC(psci_get_cpu_save_space)

@ Expects cpu ID in r0 and PC in r2, please ignore the return value.
ENTRY(psci_save_target_pc)
	push	{lr}

	@ Save target PC into stack
	bl	psci_get_cpu_save_space
	str	r2, [r0, #SAVE_SPACE_TARGET_PC_OFFSET]
	dsb

	pop	{pc}
ENDPROC(psci_save_target_pc)

@ Expects cpu ID in r0 and context ID in r3, please ignore the return value.
ENTRY(psci_save_cpu_context_id)
	push	{lr}

	@ Save context id
	bl	psci_get_cpu_save_space
	str	r3, [r0, #SAVE_SPACE_CONTEXT_ID_OFFSET]
	dsb

	pop	{pc}
ENDPROC(psci_save_cpu_context_id)

@ Expect cpu ID in r0, return context ID in r0.
ENTRY(psci_get_cpu_context_id)
	push	{lr}

	@ Get context id
	bl	psci_get_cpu_save_space
	ldr	r0, [r0, #SAVE_SPACE_CONTEXT_ID_OFFSET]

	pop	{pc}
ENDPROC(psci_get_cpu_context_id)

ENTRY(psci_cpu_entry)
	bl	psci_enable_smp

	bl	_nonsec_init

	bl	psci_get_cpu_id			@ CPU ID => r0
	bl	psci_get_cpu_save_space
	ldr	r0, [r0, #SAVE_SPACE_TARGET_PC_OFFSET]
	b	_do_nonsec_entry
ENDPROC(psci_cpu_entry)

	.popsection
