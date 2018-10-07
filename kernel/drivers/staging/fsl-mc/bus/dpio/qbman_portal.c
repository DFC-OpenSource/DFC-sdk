/* Copyright (C) 2014 Freescale Semiconductor, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Freescale Semiconductor nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 *
 * ALTERNATIVELY, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") as published by the Free Software
 * Foundation, either version 2 of that License or (at your option) any
 * later version.
 *
 * THIS SOFTWARE IS PROVIDED BY Freescale Semiconductor ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Freescale Semiconductor BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "qbman_portal.h"

/* QBMan portal management command codes */
#define QBMAN_MC_ACQUIRE       0x30
#define QBMAN_WQCHAN_CONFIGURE 0x46

/* CINH register offsets */
#define QBMAN_CINH_SWP_EQAR    0x8c0
#define QBMAN_CINH_SWP_DQPI    0xa00
#define QBMAN_CINH_SWP_DCAP    0xac0
#define QBMAN_CINH_SWP_SDQCR   0xb00
#define QBMAN_CINH_SWP_RAR     0xcc0
#define QBMAN_CINH_SWP_ISR     0xe00
#define QBMAN_CINH_SWP_IER     0xe40
#define QBMAN_CINH_SWP_ISDR    0xe80
#define QBMAN_CINH_SWP_IIR     0xec0

/* CENA register offsets */
#define QBMAN_CENA_SWP_EQCR(n) (0x000 + ((uint32_t)(n) << 6))
#define QBMAN_CENA_SWP_DQRR(n) (0x200 + ((uint32_t)(n) << 6))
#define QBMAN_CENA_SWP_RCR(n)  (0x400 + ((uint32_t)(n) << 6))
#define QBMAN_CENA_SWP_CR      0x600
#define QBMAN_CENA_SWP_RR(vb)  (0x700 + ((uint32_t)(vb) >> 1))
#define QBMAN_CENA_SWP_VDQCR   0x780

/* Reverse mapping of QBMAN_CENA_SWP_DQRR() */
#define QBMAN_IDX_FROM_DQRR(p) (((unsigned long)p & 0x1ff) >> 6)

/* QBMan FQ management command codes */
#define QBMAN_FQ_SCHEDULE	0x48
#define QBMAN_FQ_FORCE		0x49
#define QBMAN_FQ_XON		0x4d
#define QBMAN_FQ_XOFF		0x4e

/*******************************/
/* Pre-defined attribute codes */
/*******************************/

struct qb_attr_code code_generic_verb = QB_CODE(0, 0, 7);
struct qb_attr_code code_generic_rslt = QB_CODE(0, 8, 8);

/*************************/
/* SDQCR attribute codes */
/*************************/

/* we put these here because at least some of them are required by
 * qbman_swp_init() */
struct qb_attr_code code_sdqcr_dct = QB_CODE(0, 24, 2);
struct qb_attr_code code_sdqcr_fc = QB_CODE(0, 29, 1);
struct qb_attr_code code_sdqcr_tok = QB_CODE(0, 16, 8);
#define CODE_SDQCR_DQSRC(n) QB_CODE(0, n, 1)
enum qbman_sdqcr_dct {
	qbman_sdqcr_dct_null = 0,
	qbman_sdqcr_dct_prio_ics,
	qbman_sdqcr_dct_active_ics,
	qbman_sdqcr_dct_active
};
enum qbman_sdqcr_fc {
	qbman_sdqcr_fc_one = 0,
	qbman_sdqcr_fc_up_to_3 = 1
};
struct qb_attr_code code_sdqcr_dqsrc = QB_CODE(0, 0, 16);

/*********************************/
/* Portal constructor/destructor */
/*********************************/

/* Software portals should always be in the power-on state when we initialise,
 * due to the CCSR-based portal reset functionality that MC has.
 *
 * Erk! Turns out that QMan versions prior to 4.1 do not correctly reset DQRR
 * valid-bits, so we need to support a workaround where we don't trust
 * valid-bits when detecting new entries until any stale ring entries have been
 * overwritten at least once. The idea is that we read PI for the first few
 * entries, then switch to valid-bit after that. The trick is to clear the
 * bug-work-around boolean once the PI wraps around the ring for the first time.
 *
 * Note: this still carries a slight additional cost once the decrementer hits
 * zero, so ideally the workaround should only be compiled in if the compiled
 * image needs to support affected chips. We use WORKAROUND_DQRR_RESET_BUG for
 * this.
 */
struct qbman_swp *qbman_swp_init(const struct qbman_swp_desc *d)
{
	int ret;
	struct qbman_swp *p = kmalloc(sizeof(*p), GFP_KERNEL);

	if (!p)
		return NULL;
	p->desc = d;
#ifdef QBMAN_CHECKING
	p->mc.check = swp_mc_can_start;
#endif
	p->mc.valid_bit = QB_VALID_BIT;
	p->sdq = 0;
	qb_attr_code_encode(&code_sdqcr_dct, &p->sdq, qbman_sdqcr_dct_prio_ics);
	qb_attr_code_encode(&code_sdqcr_fc, &p->sdq, qbman_sdqcr_fc_up_to_3);
	qb_attr_code_encode(&code_sdqcr_tok, &p->sdq, 0xbb);
	atomic_set(&p->vdq.busy, 1);
	p->vdq.valid_bit = QB_VALID_BIT;
	p->dqrr.next_idx = 0;
	p->dqrr.valid_bit = QB_VALID_BIT;
	/* TODO: should also read PI/CI type registers and check that they're on
	 * PoR values. If we're asked to initialise portals that aren't in reset
	 * state, bad things will follow. */
#ifdef WORKAROUND_DQRR_RESET_BUG
	p->dqrr.reset_bug = 1;
#endif
	if ((p->desc->qman_version & 0xFFFF0000) < QMAN_REV_4100)
		p->dqrr.dqrr_size = 4;
	else
		p->dqrr.dqrr_size = 8;
	ret = qbman_swp_sys_init(&p->sys, d, p->dqrr.dqrr_size);
	if (ret) {
		kfree(p);
		pr_err("qbman_swp_sys_init() failed %d\n", ret);
		return NULL;
	}
	/* SDQCR needs to be initialized to 0 when no channels are
	   being dequeued from or else the QMan HW will indicate an
	   error.  The values that were calculated above will be
	   applied when dequeues from a specific channel are enabled */
	qbman_cinh_write(&p->sys, QBMAN_CINH_SWP_SDQCR, 0);
	return p;
}

void qbman_swp_finish(struct qbman_swp *p)
{
#ifdef QBMAN_CHECKING
	BUG_ON(p->mc.check != swp_mc_can_start);
#endif
	qbman_swp_sys_finish(&p->sys);
	kfree(p);
}

const struct qbman_swp_desc *qbman_swp_get_desc(struct qbman_swp *p)
{
	return p->desc;
}

/**************/
/* Interrupts */
/**************/

uint32_t qbman_swp_interrupt_get_vanish(struct qbman_swp *p)
{
	return qbman_cinh_read(&p->sys, QBMAN_CINH_SWP_ISDR);
}

void qbman_swp_interrupt_set_vanish(struct qbman_swp *p, uint32_t mask)
{
	qbman_cinh_write(&p->sys, QBMAN_CINH_SWP_ISDR, mask);
}

uint32_t qbman_swp_interrupt_read_status(struct qbman_swp *p)
{
	return qbman_cinh_read(&p->sys, QBMAN_CINH_SWP_ISR);
}

void qbman_swp_interrupt_clear_status(struct qbman_swp *p, uint32_t mask)
{
	qbman_cinh_write(&p->sys, QBMAN_CINH_SWP_ISR, mask);
}

uint32_t qbman_swp_interrupt_get_trigger(struct qbman_swp *p)
{
	return qbman_cinh_read(&p->sys, QBMAN_CINH_SWP_IER);
}

void qbman_swp_interrupt_set_trigger(struct qbman_swp *p, uint32_t mask)
{
	qbman_cinh_write(&p->sys, QBMAN_CINH_SWP_IER, mask);
}

int qbman_swp_interrupt_get_inhibit(struct qbman_swp *p)
{
	return qbman_cinh_read(&p->sys, QBMAN_CINH_SWP_IIR);
}

void qbman_swp_interrupt_set_inhibit(struct qbman_swp *p, int inhibit)
{
	qbman_cinh_write(&p->sys, QBMAN_CINH_SWP_IIR, inhibit ? 0xffffffff : 0);
}

/***********************/
/* Management commands */
/***********************/

/*
 * Internal code common to all types of management commands.
 */

void *qbman_swp_mc_start(struct qbman_swp *p)
{
	void *ret;
#ifdef QBMAN_CHECKING
	BUG_ON(p->mc.check != swp_mc_can_start);
#endif
	ret = qbman_cena_write_start(&p->sys, QBMAN_CENA_SWP_CR);
#ifdef QBMAN_CHECKING
	if (!ret)
		p->mc.check = swp_mc_can_submit;
#endif
	return ret;
}

void qbman_swp_mc_submit(struct qbman_swp *p, void *cmd, uint32_t cmd_verb)
{
	uint32_t *v = cmd;
#ifdef QBMAN_CHECKING
	BUG_ON(!p->mc.check != swp_mc_can_submit);
#endif
	/* TBD: "|=" is going to hurt performance. Need to move as many fields
	 * out of word zero, and for those that remain, the "OR" needs to occur
	 * at the caller side. This debug check helps to catch cases where the
	 * caller wants to OR but has forgotten to do so. */
	BUG_ON((*v & cmd_verb) != *v);
	*v = cmd_verb | p->mc.valid_bit;
	qbman_cena_write_complete(&p->sys, QBMAN_CENA_SWP_CR, cmd);
#ifdef QBMAN_CHECKING
	p->mc.check = swp_mc_can_poll;
#endif
}

void *qbman_swp_mc_result(struct qbman_swp *p)
{
	uint32_t *ret, verb;
#ifdef QBMAN_CHECKING
	BUG_ON(p->mc.check != swp_mc_can_poll);
#endif
	qbman_cena_invalidate_prefetch(&p->sys,
				 QBMAN_CENA_SWP_RR(p->mc.valid_bit));
	ret = qbman_cena_read(&p->sys, QBMAN_CENA_SWP_RR(p->mc.valid_bit));
	/* Remove the valid-bit - command completed iff the rest is non-zero */
	verb = ret[0] & ~QB_VALID_BIT;
	if (!verb)
		return NULL;
#ifdef QBMAN_CHECKING
	p->mc.check = swp_mc_can_start;
#endif
	p->mc.valid_bit ^= QB_VALID_BIT;
	return ret;
}

/***********/
/* Enqueue */
/***********/

/* These should be const, eventually */
static struct qb_attr_code code_eq_cmd = QB_CODE(0, 0, 2);
static struct qb_attr_code code_eq_eqdi = QB_CODE(0, 3, 1);
static struct qb_attr_code code_eq_dca_en = QB_CODE(0, 15, 1);
static struct qb_attr_code code_eq_dca_pk = QB_CODE(0, 14, 1);
static struct qb_attr_code code_eq_dca_idx = QB_CODE(0, 8, 2);
static struct qb_attr_code code_eq_orp_en = QB_CODE(0, 2, 1);
static struct qb_attr_code code_eq_orp_is_nesn = QB_CODE(0, 31, 1);
static struct qb_attr_code code_eq_orp_nlis = QB_CODE(0, 30, 1);
static struct qb_attr_code code_eq_orp_seqnum = QB_CODE(0, 16, 14);
static struct qb_attr_code code_eq_opr_id = QB_CODE(1, 0, 16);
static struct qb_attr_code code_eq_tgt_id = QB_CODE(2, 0, 24);
/* static struct qb_attr_code code_eq_tag = QB_CODE(3, 0, 32); */
static struct qb_attr_code code_eq_qd_en = QB_CODE(0, 4, 1);
static struct qb_attr_code code_eq_qd_bin = QB_CODE(4, 0, 16);
static struct qb_attr_code code_eq_qd_pri = QB_CODE(4, 16, 4);
static struct qb_attr_code code_eq_rsp_stash = QB_CODE(5, 16, 1);
static struct qb_attr_code code_eq_rsp_id = QB_CODE(5, 24, 8);
static struct qb_attr_code code_eq_rsp_lo = QB_CODE(6, 0, 32);

enum qbman_eq_cmd_e {
	/* No enqueue, primarily for plugging ORP gaps for dropped frames */
	qbman_eq_cmd_empty,
	/* DMA an enqueue response once complete */
	qbman_eq_cmd_respond,
	/* DMA an enqueue response only if the enqueue fails */
	qbman_eq_cmd_respond_reject
};

void qbman_eq_desc_clear(struct qbman_eq_desc *d)
{
	memset(d, 0, sizeof(*d));
}

void qbman_eq_desc_set_no_orp(struct qbman_eq_desc *d, int respond_success)
{
	uint32_t *cl = qb_cl(d);

	qb_attr_code_encode(&code_eq_orp_en, cl, 0);
	qb_attr_code_encode(&code_eq_cmd, cl,
			    respond_success ? qbman_eq_cmd_respond :
					      qbman_eq_cmd_respond_reject);
}

void qbman_eq_desc_set_orp(struct qbman_eq_desc *d, int respond_success,
			   uint32_t opr_id, uint32_t seqnum, int incomplete)
{
	uint32_t *cl = qb_cl(d);

	qb_attr_code_encode(&code_eq_orp_en, cl, 1);
	qb_attr_code_encode(&code_eq_cmd, cl,
			    respond_success ? qbman_eq_cmd_respond :
					      qbman_eq_cmd_respond_reject);
	qb_attr_code_encode(&code_eq_opr_id, cl, opr_id);
	qb_attr_code_encode(&code_eq_orp_seqnum, cl, seqnum);
	qb_attr_code_encode(&code_eq_orp_nlis, cl, !!incomplete);
}

void qbman_eq_desc_set_orp_hole(struct qbman_eq_desc *d, uint32_t opr_id,
				uint32_t seqnum)
{
	uint32_t *cl = qb_cl(d);

	qb_attr_code_encode(&code_eq_orp_en, cl, 1);
	qb_attr_code_encode(&code_eq_cmd, cl, qbman_eq_cmd_empty);
	qb_attr_code_encode(&code_eq_opr_id, cl, opr_id);
	qb_attr_code_encode(&code_eq_orp_seqnum, cl, seqnum);
	qb_attr_code_encode(&code_eq_orp_nlis, cl, 0);
	qb_attr_code_encode(&code_eq_orp_is_nesn, cl, 0);
}

void qbman_eq_desc_set_orp_nesn(struct qbman_eq_desc *d, uint32_t opr_id,
				uint32_t seqnum)
{
	uint32_t *cl = qb_cl(d);

	qb_attr_code_encode(&code_eq_orp_en, cl, 1);
	qb_attr_code_encode(&code_eq_cmd, cl, qbman_eq_cmd_empty);
	qb_attr_code_encode(&code_eq_opr_id, cl, opr_id);
	qb_attr_code_encode(&code_eq_orp_seqnum, cl, seqnum);
	qb_attr_code_encode(&code_eq_orp_nlis, cl, 0);
	qb_attr_code_encode(&code_eq_orp_is_nesn, cl, 1);
}

void qbman_eq_desc_set_response(struct qbman_eq_desc *d,
				dma_addr_t storage_phys,
				int stash)
{
	uint32_t *cl = qb_cl(d);

	qb_attr_code_encode_64(&code_eq_rsp_lo, (uint64_t *)cl, storage_phys);
	qb_attr_code_encode(&code_eq_rsp_stash, cl, !!stash);
}

void qbman_eq_desc_set_token(struct qbman_eq_desc *d, uint8_t token)
{
	uint32_t *cl = qb_cl(d);

	qb_attr_code_encode(&code_eq_rsp_id, cl, (uint32_t)token);
}

void qbman_eq_desc_set_fq(struct qbman_eq_desc *d, uint32_t fqid)
{
	uint32_t *cl = qb_cl(d);

	qb_attr_code_encode(&code_eq_qd_en, cl, 0);
	qb_attr_code_encode(&code_eq_tgt_id, cl, fqid);
}

void qbman_eq_desc_set_qd(struct qbman_eq_desc *d, uint32_t qdid,
			  uint32_t qd_bin, uint32_t qd_prio)
{
	uint32_t *cl = qb_cl(d);

	qb_attr_code_encode(&code_eq_qd_en, cl, 1);
	qb_attr_code_encode(&code_eq_tgt_id, cl, qdid);
	qb_attr_code_encode(&code_eq_qd_bin, cl, qd_bin);
	qb_attr_code_encode(&code_eq_qd_pri, cl, qd_prio);
}

void qbman_eq_desc_set_eqdi(struct qbman_eq_desc *d, int enable)
{
	uint32_t *cl = qb_cl(d);

	qb_attr_code_encode(&code_eq_eqdi, cl, !!enable);
}

void qbman_eq_desc_set_dca(struct qbman_eq_desc *d, int enable,
				uint32_t dqrr_idx, int park)
{
	uint32_t *cl = qb_cl(d);

	qb_attr_code_encode(&code_eq_dca_en, cl, !!enable);
	if (enable) {
		qb_attr_code_encode(&code_eq_dca_pk, cl, !!park);
		qb_attr_code_encode(&code_eq_dca_idx, cl, dqrr_idx);
	}
}

#define EQAR_IDX(eqar)     ((eqar) & 0x7)
#define EQAR_VB(eqar)      ((eqar) & 0x80)
#define EQAR_SUCCESS(eqar) ((eqar) & 0x100)

int qbman_swp_enqueue(struct qbman_swp *s, const struct qbman_eq_desc *d,
		      const struct qbman_fd *fd)
{
	uint32_t *p;
	const uint32_t *cl = qb_cl(d);
	uint32_t eqar = qbman_cinh_read(&s->sys, QBMAN_CINH_SWP_EQAR);

	pr_debug("EQAR=%08x\n", eqar);
	if (!EQAR_SUCCESS(eqar))
		return -EBUSY;
	p = qbman_cena_write_start(&s->sys,
				   QBMAN_CENA_SWP_EQCR(EQAR_IDX(eqar)));
	word_copy(&p[1], &cl[1], 7);
	word_copy(&p[8], fd, sizeof(*fd) >> 2);
	/* Set the verb byte, have to substitute in the valid-bit */
	p[0] = cl[0] | EQAR_VB(eqar);
	qbman_cena_write_complete(&s->sys,
				  QBMAN_CENA_SWP_EQCR(EQAR_IDX(eqar)),
				  p);
	return 0;
}

/*************************/
/* Static (push) dequeue */
/*************************/

void qbman_swp_push_get(struct qbman_swp *s, uint8_t channel_idx, int *enabled)
{
	struct qb_attr_code code = CODE_SDQCR_DQSRC(channel_idx);

	BUG_ON(channel_idx > 15);
	*enabled = (int)qb_attr_code_decode(&code, &s->sdq);
}

void qbman_swp_push_set(struct qbman_swp *s, uint8_t channel_idx, int enable)
{
	uint16_t dqsrc;
	struct qb_attr_code code = CODE_SDQCR_DQSRC(channel_idx);

	BUG_ON(channel_idx > 15);
	qb_attr_code_encode(&code, &s->sdq, !!enable);
	/* Read make the complete src map.  If no channels are enabled
	   the SDQCR must be 0 or else QMan will assert errors */
	dqsrc = (uint16_t)qb_attr_code_decode(&code_sdqcr_dqsrc, &s->sdq);
	if (dqsrc != 0)
		qbman_cinh_write(&s->sys, QBMAN_CINH_SWP_SDQCR, s->sdq);
	else
		qbman_cinh_write(&s->sys, QBMAN_CINH_SWP_SDQCR, 0);
}

/***************************/
/* Volatile (pull) dequeue */
/***************************/

/* These should be const, eventually */
static struct qb_attr_code code_pull_dct = QB_CODE(0, 0, 2);
static struct qb_attr_code code_pull_dt = QB_CODE(0, 2, 2);
static struct qb_attr_code code_pull_rls = QB_CODE(0, 4, 1);
static struct qb_attr_code code_pull_stash = QB_CODE(0, 5, 1);
static struct qb_attr_code code_pull_numframes = QB_CODE(0, 8, 4);
static struct qb_attr_code code_pull_token = QB_CODE(0, 16, 8);
static struct qb_attr_code code_pull_dqsource = QB_CODE(1, 0, 24);
static struct qb_attr_code code_pull_rsp_lo = QB_CODE(2, 0, 32);

enum qb_pull_dt_e {
	qb_pull_dt_channel,
	qb_pull_dt_workqueue,
	qb_pull_dt_framequeue
};

void qbman_pull_desc_clear(struct qbman_pull_desc *d)
{
	memset(d, 0, sizeof(*d));
}

void qbman_pull_desc_set_storage(struct qbman_pull_desc *d,
				 struct dpaa2_dq *storage,
				 dma_addr_t storage_phys,
				 int stash)
{
	uint32_t *cl = qb_cl(d);

	/* Squiggle the pointer 'storage' into the extra 2 words of the
	 * descriptor (which aren't copied to the hw command) */
	*(void **)&cl[4] = storage;
	if (!storage) {
		qb_attr_code_encode(&code_pull_rls, cl, 0);
		return;
	}
	qb_attr_code_encode(&code_pull_rls, cl, 1);
	qb_attr_code_encode(&code_pull_stash, cl, !!stash);
	qb_attr_code_encode_64(&code_pull_rsp_lo, (uint64_t *)cl, storage_phys);
}

void qbman_pull_desc_set_numframes(struct qbman_pull_desc *d, uint8_t numframes)
{
	uint32_t *cl = qb_cl(d);

	BUG_ON(!numframes || (numframes > 16));
	qb_attr_code_encode(&code_pull_numframes, cl,
				(uint32_t)(numframes - 1));
}

void qbman_pull_desc_set_token(struct qbman_pull_desc *d, uint8_t token)
{
	uint32_t *cl = qb_cl(d);

	qb_attr_code_encode(&code_pull_token, cl, token);
}

void qbman_pull_desc_set_fq(struct qbman_pull_desc *d, uint32_t fqid)
{
	uint32_t *cl = qb_cl(d);

	qb_attr_code_encode(&code_pull_dct, cl, 1);
	qb_attr_code_encode(&code_pull_dt, cl, qb_pull_dt_framequeue);
	qb_attr_code_encode(&code_pull_dqsource, cl, fqid);
}

void qbman_pull_desc_set_wq(struct qbman_pull_desc *d, uint32_t wqid,
			    enum qbman_pull_type_e dct)
{
	uint32_t *cl = qb_cl(d);

	qb_attr_code_encode(&code_pull_dct, cl, dct);
	qb_attr_code_encode(&code_pull_dt, cl, qb_pull_dt_workqueue);
	qb_attr_code_encode(&code_pull_dqsource, cl, wqid);
}

void qbman_pull_desc_set_channel(struct qbman_pull_desc *d, uint32_t chid,
				 enum qbman_pull_type_e dct)
{
	uint32_t *cl = qb_cl(d);

	qb_attr_code_encode(&code_pull_dct, cl, dct);
	qb_attr_code_encode(&code_pull_dt, cl, qb_pull_dt_channel);
	qb_attr_code_encode(&code_pull_dqsource, cl, chid);
}

int qbman_swp_pull(struct qbman_swp *s, struct qbman_pull_desc *d)
{
	uint32_t *p;
	uint32_t *cl = qb_cl(d);

	if (!atomic_dec_and_test(&s->vdq.busy)) {
		atomic_inc(&s->vdq.busy);
		return -EBUSY;
	}
	s->vdq.storage = *(void **)&cl[4];
	qb_attr_code_encode(&code_pull_token, cl, 1);
	p = qbman_cena_write_start(&s->sys, QBMAN_CENA_SWP_VDQCR);
	word_copy(&p[1], &cl[1], 3);
	/* Set the verb byte, have to substitute in the valid-bit */
	p[0] = cl[0] | s->vdq.valid_bit;
	s->vdq.valid_bit ^= QB_VALID_BIT;
	qbman_cena_write_complete(&s->sys, QBMAN_CENA_SWP_VDQCR, p);
	return 0;
}

/****************/
/* Polling DQRR */
/****************/

static struct qb_attr_code code_dqrr_verb = QB_CODE(0, 0, 8);
static struct qb_attr_code code_dqrr_response = QB_CODE(0, 0, 7);
static struct qb_attr_code code_dqrr_stat = QB_CODE(0, 8, 8);
static struct qb_attr_code code_dqrr_seqnum = QB_CODE(0, 16, 14);
static struct qb_attr_code code_dqrr_odpid = QB_CODE(1, 0, 16);
/* static struct qb_attr_code code_dqrr_tok = QB_CODE(1, 24, 8); */
static struct qb_attr_code code_dqrr_fqid = QB_CODE(2, 0, 24);
static struct qb_attr_code code_dqrr_byte_count = QB_CODE(4, 0, 32);
static struct qb_attr_code code_dqrr_frame_count = QB_CODE(5, 0, 24);
static struct qb_attr_code code_dqrr_ctx_lo = QB_CODE(6, 0, 32);

#define QBMAN_RESULT_DQ        0x60
#define QBMAN_RESULT_FQRN      0x21
#define QBMAN_RESULT_FQRNI     0x22
#define QBMAN_RESULT_FQPN      0x24
#define QBMAN_RESULT_FQDAN     0x25
#define QBMAN_RESULT_CDAN      0x26
#define QBMAN_RESULT_CSCN_MEM  0x27
#define QBMAN_RESULT_CGCU      0x28
#define QBMAN_RESULT_BPSCN     0x29
#define QBMAN_RESULT_CSCN_WQ   0x2a

static struct qb_attr_code code_dqpi_pi = QB_CODE(0, 0, 4);

/* NULL return if there are no unconsumed DQRR entries. Returns a DQRR entry
 * only once, so repeated calls can return a sequence of DQRR entries, without
 * requiring they be consumed immediately or in any particular order. */
const struct dpaa2_dq *qbman_swp_dqrr_next(struct qbman_swp *s)
{
	uint32_t verb;
	uint32_t response_verb;
	uint32_t flags;
	const struct dpaa2_dq *dq;
	const uint32_t *p;

	/* Before using valid-bit to detect if something is there, we have to
	 * handle the case of the DQRR reset bug... */
#ifdef WORKAROUND_DQRR_RESET_BUG
	if (unlikely(s->dqrr.reset_bug)) {
		/* We pick up new entries by cache-inhibited producer index,
		 * which means that a non-coherent mapping would require us to
		 * invalidate and read *only* once that PI has indicated that
		 * there's an entry here. The first trip around the DQRR ring
		 * will be much less efficient than all subsequent trips around
		 * it...
		 */
		uint32_t dqpi = qbman_cinh_read(&s->sys, QBMAN_CINH_SWP_DQPI);
		uint32_t pi = qb_attr_code_decode(&code_dqpi_pi, &dqpi);
		/* there are new entries iff pi != next_idx */
		if (pi == s->dqrr.next_idx)
			return NULL;
		/* if next_idx is/was the last ring index, and 'pi' is
		 * different, we can disable the workaround as all the ring
		 * entries have now been DMA'd to so valid-bit checking is
		 * repaired. Note: this logic needs to be based on next_idx
		 * (which increments one at a time), rather than on pi (which
		 * can burst and wrap-around between our snapshots of it).
		 */
		if (s->dqrr.next_idx == (s->dqrr.dqrr_size - 1)) {
			pr_debug("DEBUG: next_idx=%d, pi=%d, clear reset bug\n",
				s->dqrr.next_idx, pi);
			s->dqrr.reset_bug = 0;
		}
		qbman_cena_invalidate_prefetch(&s->sys,
					QBMAN_CENA_SWP_DQRR(s->dqrr.next_idx));
	}
#endif

	dq = qbman_cena_read(&s->sys, QBMAN_CENA_SWP_DQRR(s->dqrr.next_idx));
	p = qb_cl(dq);
	verb = qb_attr_code_decode(&code_dqrr_verb, p);

	/* If the valid-bit isn't of the expected polarity, nothing there. Note,
	 * in the DQRR reset bug workaround, we shouldn't need to skip these
	 * check, because we've already determined that a new entry is available
	 * and we've invalidated the cacheline before reading it, so the
	 * valid-bit behaviour is repaired and should tell us what we already
	 * knew from reading PI.
	 */
	if ((verb & QB_VALID_BIT) != s->dqrr.valid_bit) {
		qbman_cena_invalidate_prefetch(&s->sys,
					QBMAN_CENA_SWP_DQRR(s->dqrr.next_idx));
		return NULL;
	}
	/* There's something there. Move "next_idx" attention to the next ring
	 * entry (and prefetch it) before returning what we found. */
	s->dqrr.next_idx++;
	s->dqrr.next_idx &= s->dqrr.dqrr_size - 1; /* Wrap around */
	/* TODO: it's possible to do all this without conditionals, optimise it
	 * later. */
	if (!s->dqrr.next_idx)
		s->dqrr.valid_bit ^= QB_VALID_BIT;

	/* If this is the final response to a volatile dequeue command
	   indicate that the vdq is no longer busy */
	flags = dpaa2_dq_flags(dq);
	response_verb = qb_attr_code_decode(&code_dqrr_response, &verb);
	if ((response_verb == QBMAN_RESULT_DQ) &&
	    (flags & DPAA2_DQ_STAT_VOLATILE) &&
	    (flags & DPAA2_DQ_STAT_EXPIRED))
		atomic_inc(&s->vdq.busy);

	qbman_cena_invalidate_prefetch(&s->sys,
				       QBMAN_CENA_SWP_DQRR(s->dqrr.next_idx));
	return dq;
}

/* Consume DQRR entries previously returned from qbman_swp_dqrr_next(). */
void qbman_swp_dqrr_consume(struct qbman_swp *s, const struct dpaa2_dq *dq)
{
	qbman_cinh_write(&s->sys, QBMAN_CINH_SWP_DCAP, QBMAN_IDX_FROM_DQRR(dq));
}

/*********************************/
/* Polling user-provided storage */
/*********************************/

int qbman_result_has_new_result(struct qbman_swp *s,
				  const struct dpaa2_dq *dq)
{
	/* To avoid converting the little-endian DQ entry to host-endian prior
	 * to us knowing whether there is a valid entry or not (and run the
	 * risk of corrupting the incoming hardware LE write), we detect in
	 * hardware endianness rather than host. This means we need a different
	 * "code" depending on whether we are BE or LE in software, which is
	 * where DQRR_TOK_OFFSET comes in... */
	static struct qb_attr_code code_dqrr_tok_detect =
					QB_CODE(0, DQRR_TOK_OFFSET, 8);
	/* The user trying to poll for a result treats "dq" as const. It is
	 * however the same address that was provided to us non-const in the
	 * first place, for directing hardware DMA to. So we can cast away the
	 * const because it is mutable from our perspective. */
	uint32_t *p = qb_cl((struct dpaa2_dq *)dq);
	uint32_t token;

	token = qb_attr_code_decode(&code_dqrr_tok_detect, &p[1]);
	if (token != 1)
		return 0;
	qb_attr_code_encode(&code_dqrr_tok_detect, &p[1], 0);

	/* Only now do we convert from hardware to host endianness. Also, as we
	 * are returning success, the user has promised not to call us again, so
	 * there's no risk of us converting the endianness twice... */
	make_le32_n(p, 16);

	/* VDQCR "no longer busy" hook - not quite the same as DQRR, because the
	 * fact "VDQCR" shows busy doesn't mean that the result we're looking at
	 * is from the same command. Eg. we may be looking at our 10th dequeue
	 * result from our first VDQCR command, yet the second dequeue command
	 * could have been kicked off already, after seeing the 1st result. Ie.
	 * the result we're looking at is not necessarily proof that we can
	 * reset "busy".  We instead base the decision on whether the current
	 * result is sitting at the first 'storage' location of the busy
	 * command. */
	if (s->vdq.storage == dq) {
		s->vdq.storage = NULL;
		atomic_inc(&s->vdq.busy);
	}
	return 1;
}

/********************************/
/* Categorising qbman_result */
/********************************/

static struct qb_attr_code code_result_in_mem =
			QB_CODE(0, QBMAN_RESULT_VERB_OFFSET_IN_MEM, 7);

static inline int __qbman_result_is_x(const struct dpaa2_dq *dq, uint32_t x)
{
	const uint32_t *p = qb_cl(dq);
	uint32_t response_verb = qb_attr_code_decode(&code_dqrr_response, p);

	return response_verb == x;
}

static inline int __qbman_result_is_x_in_mem(const struct dpaa2_dq *dq,
					     uint32_t x)
{
	const uint32_t *p = qb_cl(dq);
	uint32_t response_verb = qb_attr_code_decode(&code_result_in_mem, p);

	return (response_verb == x);
}

int qbman_result_is_DQ(const struct dpaa2_dq *dq)
{
	return __qbman_result_is_x(dq, QBMAN_RESULT_DQ);
}

int qbman_result_is_FQDAN(const struct dpaa2_dq *dq)
{
	return __qbman_result_is_x(dq, QBMAN_RESULT_FQDAN);
}

int qbman_result_is_CDAN(const struct dpaa2_dq *dq)
{
	return __qbman_result_is_x(dq, QBMAN_RESULT_CDAN);
}

int qbman_result_is_CSCN(const struct dpaa2_dq *dq)
{
	return __qbman_result_is_x_in_mem(dq, QBMAN_RESULT_CSCN_MEM) ||
		__qbman_result_is_x(dq, QBMAN_RESULT_CSCN_WQ);
}

int qbman_result_is_BPSCN(const struct dpaa2_dq *dq)
{
	return __qbman_result_is_x_in_mem(dq, QBMAN_RESULT_BPSCN);
}

int qbman_result_is_CGCU(const struct dpaa2_dq *dq)
{
	return __qbman_result_is_x_in_mem(dq, QBMAN_RESULT_CGCU);
}

int qbman_result_is_FQRN(const struct dpaa2_dq *dq)
{
	return __qbman_result_is_x_in_mem(dq, QBMAN_RESULT_FQRN);
}

int qbman_result_is_FQRNI(const struct dpaa2_dq *dq)
{
	return __qbman_result_is_x_in_mem(dq, QBMAN_RESULT_FQRNI);
}

int qbman_result_is_FQPN(const struct dpaa2_dq *dq)
{
	return __qbman_result_is_x(dq, QBMAN_RESULT_FQPN);
}

/*********************************/
/* Parsing frame dequeue results */
/*********************************/

/* These APIs assume qbman_result_is_DQ() is TRUE */

uint32_t dpaa2_dq_flags(const struct dpaa2_dq *dq)
{
	const uint32_t *p = qb_cl(dq);

	return qb_attr_code_decode(&code_dqrr_stat, p);
}

uint16_t dpaa2_dq_seqnum(const struct dpaa2_dq *dq)
{
	const uint32_t *p = qb_cl(dq);

	return (uint16_t)qb_attr_code_decode(&code_dqrr_seqnum, p);
}

uint16_t dpaa2_dq_odpid(const struct dpaa2_dq *dq)
{
	const uint32_t *p = qb_cl(dq);

	return (uint16_t)qb_attr_code_decode(&code_dqrr_odpid, p);
}

uint32_t dpaa2_dq_fqid(const struct dpaa2_dq *dq)
{
	const uint32_t *p = qb_cl(dq);

	return qb_attr_code_decode(&code_dqrr_fqid, p);
}

uint32_t dpaa2_dq_byte_count(const struct dpaa2_dq *dq)
{
	const uint32_t *p = qb_cl(dq);

	return qb_attr_code_decode(&code_dqrr_byte_count, p);
}

uint32_t dpaa2_dq_frame_count(const struct dpaa2_dq *dq)
{
	const uint32_t *p = qb_cl(dq);

	return qb_attr_code_decode(&code_dqrr_frame_count, p);
}

uint64_t dpaa2_dq_fqd_ctx(const struct dpaa2_dq *dq)
{
	const uint64_t *p = (uint64_t *)qb_cl(dq);

	return qb_attr_code_decode_64(&code_dqrr_ctx_lo, p);
}
EXPORT_SYMBOL(dpaa2_dq_fqd_ctx);

const struct dpaa2_fd *dpaa2_dq_fd(const struct dpaa2_dq *dq)
{
	const uint32_t *p = qb_cl(dq);

	return (const struct dpaa2_fd *)&p[8];
}
EXPORT_SYMBOL(dpaa2_dq_fd);

/**************************************/
/* Parsing state-change notifications */
/**************************************/

static struct qb_attr_code code_scn_state = QB_CODE(0, 16, 8);
static struct qb_attr_code code_scn_rid = QB_CODE(1, 0, 24);
static struct qb_attr_code code_scn_state_in_mem =
			QB_CODE(0, SCN_STATE_OFFSET_IN_MEM, 8);
static struct qb_attr_code code_scn_rid_in_mem =
			QB_CODE(1, SCN_RID_OFFSET_IN_MEM, 24);
static struct qb_attr_code code_scn_ctx_lo = QB_CODE(2, 0, 32);

uint8_t qbman_result_SCN_state(const struct dpaa2_dq *scn)
{
	const uint32_t *p = qb_cl(scn);

	return (uint8_t)qb_attr_code_decode(&code_scn_state, p);
}

uint32_t qbman_result_SCN_rid(const struct dpaa2_dq *scn)
{
	const uint32_t *p = qb_cl(scn);

	return qb_attr_code_decode(&code_scn_rid, p);
}

uint64_t qbman_result_SCN_ctx(const struct dpaa2_dq *scn)
{
	const uint64_t *p = (uint64_t *)qb_cl(scn);

	return qb_attr_code_decode_64(&code_scn_ctx_lo, p);
}

uint8_t qbman_result_SCN_state_in_mem(const struct dpaa2_dq *scn)
{
	const uint32_t *p = qb_cl(scn);

	return (uint8_t)qb_attr_code_decode(&code_scn_state_in_mem, p);
}

uint32_t qbman_result_SCN_rid_in_mem(const struct dpaa2_dq *scn)
{
	const uint32_t *p = qb_cl(scn);
	uint32_t result_rid;

	result_rid = qb_attr_code_decode(&code_scn_rid_in_mem, p);
	return make_le24(result_rid);
}

/*****************/
/* Parsing BPSCN */
/*****************/
uint16_t qbman_result_bpscn_bpid(const struct dpaa2_dq *scn)
{
	return (uint16_t)qbman_result_SCN_rid_in_mem(scn) & 0x3FFF;
}

int qbman_result_bpscn_has_free_bufs(const struct dpaa2_dq *scn)
{
	return !(int)(qbman_result_SCN_state_in_mem(scn) & 0x1);
}

int qbman_result_bpscn_is_depleted(const struct dpaa2_dq *scn)
{
	return (int)(qbman_result_SCN_state_in_mem(scn) & 0x2);
}

int qbman_result_bpscn_is_surplus(const struct dpaa2_dq *scn)
{
	return (int)(qbman_result_SCN_state_in_mem(scn) & 0x4);
}

uint64_t qbman_result_bpscn_ctx(const struct dpaa2_dq *scn)
{
	return qbman_result_SCN_ctx(scn);
}

/*****************/
/* Parsing CGCU  */
/*****************/
uint16_t qbman_result_cgcu_cgid(const struct dpaa2_dq *scn)
{
	return (uint16_t)qbman_result_SCN_rid_in_mem(scn) & 0xFFFF;
}

uint64_t qbman_result_cgcu_icnt(const struct dpaa2_dq *scn)
{
	return qbman_result_SCN_ctx(scn) & 0xFFFFFFFFFF;
}

/******************/
/* Buffer release */
/******************/

/* These should be const, eventually */
/* static struct qb_attr_code code_release_num = QB_CODE(0, 0, 3); */
static struct qb_attr_code code_release_set_me = QB_CODE(0, 5, 1);
static struct qb_attr_code code_release_rcdi = QB_CODE(0, 6, 1);
static struct qb_attr_code code_release_bpid = QB_CODE(0, 16, 16);

void qbman_release_desc_clear(struct qbman_release_desc *d)
{
	uint32_t *cl;

	memset(d, 0, sizeof(*d));
	cl = qb_cl(d);
	qb_attr_code_encode(&code_release_set_me, cl, 1);
}

void qbman_release_desc_set_bpid(struct qbman_release_desc *d, uint32_t bpid)
{
	uint32_t *cl = qb_cl(d);

	qb_attr_code_encode(&code_release_bpid, cl, bpid);
}

void qbman_release_desc_set_rcdi(struct qbman_release_desc *d, int enable)
{
	uint32_t *cl = qb_cl(d);

	qb_attr_code_encode(&code_release_rcdi, cl, !!enable);
}

#define RAR_IDX(rar)     ((rar) & 0x7)
#define RAR_VB(rar)      ((rar) & 0x80)
#define RAR_SUCCESS(rar) ((rar) & 0x100)

int qbman_swp_release(struct qbman_swp *s, const struct qbman_release_desc *d,
		      const uint64_t *buffers, unsigned int num_buffers)
{
	uint32_t *p;
	const uint32_t *cl = qb_cl(d);
	uint32_t rar = qbman_cinh_read(&s->sys, QBMAN_CINH_SWP_RAR);

	pr_debug("RAR=%08x\n", rar);
	if (!RAR_SUCCESS(rar))
		return -EBUSY;
	BUG_ON(!num_buffers || (num_buffers > 7));
	/* Start the release command */
	p = qbman_cena_write_start(&s->sys,
				   QBMAN_CENA_SWP_RCR(RAR_IDX(rar)));
	/* Copy the caller's buffer pointers to the command */
	u64_to_le32_copy(&p[2], buffers, num_buffers);
	/* Set the verb byte, have to substitute in the valid-bit and the number
	 * of buffers. */
	p[0] = cl[0] | RAR_VB(rar) | num_buffers;
	qbman_cena_write_complete(&s->sys,
				  QBMAN_CENA_SWP_RCR(RAR_IDX(rar)),
				  p);
	return 0;
}

/*******************/
/* Buffer acquires */
/*******************/

/* These should be const, eventually */
static struct qb_attr_code code_acquire_bpid = QB_CODE(0, 16, 16);
static struct qb_attr_code code_acquire_num = QB_CODE(1, 0, 3);
static struct qb_attr_code code_acquire_r_num = QB_CODE(1, 0, 3);

int qbman_swp_acquire(struct qbman_swp *s, uint32_t bpid, uint64_t *buffers,
		      unsigned int num_buffers)
{
	uint32_t *p;
	uint32_t verb, rslt, num;

	BUG_ON(!num_buffers || (num_buffers > 7));

	/* Start the management command */
	p = qbman_swp_mc_start(s);

	if (!p)
		return -EBUSY;

	/* Encode the caller-provided attributes */
	qb_attr_code_encode(&code_acquire_bpid, p, bpid);
	qb_attr_code_encode(&code_acquire_num, p, num_buffers);

	/* Complete the management command */
	p = qbman_swp_mc_complete(s, p, p[0] | QBMAN_MC_ACQUIRE);

	/* Decode the outcome */
	verb = qb_attr_code_decode(&code_generic_verb, p);
	rslt = qb_attr_code_decode(&code_generic_rslt, p);
	num = qb_attr_code_decode(&code_acquire_r_num, p);
	BUG_ON(verb != QBMAN_MC_ACQUIRE);

	/* Determine success or failure */
	if (unlikely(rslt != QBMAN_MC_RSLT_OK)) {
		pr_err("Acquire buffers from BPID 0x%x failed, code=0x%02x\n",
								bpid, rslt);
		return -EIO;
	}
	BUG_ON(num > num_buffers);
	/* Copy the acquired buffers to the caller's array */
	u64_from_le32_copy(buffers, &p[2], num);
	return (int)num;
}

/*****************/
/* FQ management */
/*****************/

static struct qb_attr_code code_fqalt_fqid = QB_CODE(1, 0, 32);

static int qbman_swp_alt_fq_state(struct qbman_swp *s, uint32_t fqid,
				 uint8_t alt_fq_verb)
{
	uint32_t *p;
	uint32_t verb, rslt;

	/* Start the management command */
	p = qbman_swp_mc_start(s);
	if (!p)
		return -EBUSY;

	qb_attr_code_encode(&code_fqalt_fqid, p, fqid);
	/* Complete the management command */
	p = qbman_swp_mc_complete(s, p, p[0] | alt_fq_verb);

	/* Decode the outcome */
	verb = qb_attr_code_decode(&code_generic_verb, p);
	rslt = qb_attr_code_decode(&code_generic_rslt, p);
	BUG_ON(verb != alt_fq_verb);

	/* Determine success or failure */
	if (unlikely(rslt != QBMAN_MC_RSLT_OK)) {
		pr_err("ALT FQID %d failed: verb = 0x%08x, code = 0x%02x\n",
		       fqid, alt_fq_verb, rslt);
		return -EIO;
	}

	return 0;
}

int qbman_swp_fq_schedule(struct qbman_swp *s, uint32_t fqid)
{
	return qbman_swp_alt_fq_state(s, fqid, QBMAN_FQ_SCHEDULE);
}

int qbman_swp_fq_force(struct qbman_swp *s, uint32_t fqid)
{
	return qbman_swp_alt_fq_state(s, fqid, QBMAN_FQ_FORCE);
}

int qbman_swp_fq_xon(struct qbman_swp *s, uint32_t fqid)
{
	return qbman_swp_alt_fq_state(s, fqid, QBMAN_FQ_XON);
}

int qbman_swp_fq_xoff(struct qbman_swp *s, uint32_t fqid)
{
	return qbman_swp_alt_fq_state(s, fqid, QBMAN_FQ_XOFF);
}

/**********************/
/* Channel management */
/**********************/

static struct qb_attr_code code_cdan_cid = QB_CODE(0, 16, 12);
static struct qb_attr_code code_cdan_we = QB_CODE(1, 0, 8);
static struct qb_attr_code code_cdan_en = QB_CODE(1, 8, 1);
static struct qb_attr_code code_cdan_ctx_lo = QB_CODE(2, 0, 32);

/* Hide "ICD" for now as we don't use it, don't set it, and don't test it, so it
 * would be irresponsible to expose it. */
#define CODE_CDAN_WE_EN    0x1
#define CODE_CDAN_WE_CTX   0x4

static int qbman_swp_CDAN_set(struct qbman_swp *s, uint16_t channelid,
			      uint8_t we_mask, uint8_t cdan_en,
			      uint64_t ctx)
{
	uint32_t *p;
	uint32_t verb, rslt;

	/* Start the management command */
	p = qbman_swp_mc_start(s);
	if (!p)
		return -EBUSY;

	/* Encode the caller-provided attributes */
	qb_attr_code_encode(&code_cdan_cid, p, channelid);
	qb_attr_code_encode(&code_cdan_we, p, we_mask);
	qb_attr_code_encode(&code_cdan_en, p, cdan_en);
	qb_attr_code_encode_64(&code_cdan_ctx_lo, (uint64_t *)p, ctx);
	/* Complete the management command */
	p = qbman_swp_mc_complete(s, p, p[0] | QBMAN_WQCHAN_CONFIGURE);

	/* Decode the outcome */
	verb = qb_attr_code_decode(&code_generic_verb, p);
	rslt = qb_attr_code_decode(&code_generic_rslt, p);
	BUG_ON(verb != QBMAN_WQCHAN_CONFIGURE);

	/* Determine success or failure */
	if (unlikely(rslt != QBMAN_MC_RSLT_OK)) {
		pr_err("CDAN cQID %d failed: code = 0x%02x\n",
		       channelid, rslt);
		return -EIO;
	}

	return 0;
}

int qbman_swp_CDAN_set_context(struct qbman_swp *s, uint16_t channelid,
			       uint64_t ctx)
{
	return qbman_swp_CDAN_set(s, channelid,
				  CODE_CDAN_WE_CTX,
				  0, ctx);
}

int qbman_swp_CDAN_enable(struct qbman_swp *s, uint16_t channelid)
{
	return qbman_swp_CDAN_set(s, channelid,
				  CODE_CDAN_WE_EN,
				  1, 0);
}
int qbman_swp_CDAN_disable(struct qbman_swp *s, uint16_t channelid)
{
	return qbman_swp_CDAN_set(s, channelid,
				  CODE_CDAN_WE_EN,
				  0, 0);
}
int qbman_swp_CDAN_set_context_enable(struct qbman_swp *s, uint16_t channelid,
				      uint64_t ctx)
{
	return qbman_swp_CDAN_set(s, channelid,
				  CODE_CDAN_WE_EN | CODE_CDAN_WE_CTX,
				  1, ctx);
}
