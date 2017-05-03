/*
The MIT License (MIT)

Copyright (c) 2014-2015 CSAIL, MIT

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
/*
#include <linux/module.h>
#include <linux/slab.h>
*/
#include "../bdbm_drv.h"
#include "../params.h"
#include "../platform.h"
#include "../debug.h"
#include "abm.h"
#include "../../uatomic.h"
#include <syslog.h>
/*#include "utils/file.h" */

extern atomic_t act_desc_cnt;
uint64_t abm_info_size;
static inline 
uint64_t __get_channel_ofs (struct nand_params* np, uint64_t blk_idx) {
	return (blk_idx / np->nr_blocks_per_channel);
}

static inline 
uint64_t __get_chip_ofs (struct nand_params* np, uint64_t blk_idx) {
	return ((blk_idx % np->nr_blocks_per_channel) / np->nr_blocks_per_chip);
}

static inline 
uint64_t __get_block_ofs (struct nand_params* np, uint64_t blk_idx) {
	return (blk_idx % np->nr_blocks_per_chip);
}

static inline
uint64_t __get_block_idx (struct nand_params* np, uint64_t channel_no, uint64_t chip_no, uint64_t block_no) {
	return channel_no * np->nr_blocks_per_channel + 
		chip_no * np->nr_blocks_per_chip + 
		block_no;
}

static inline
void __bdbm_abm_check_status (struct bdbm_abm_info* bai)
{
	bdbm_bug_on (bai->nr_total_blks != 
		bai->nr_free_blks + 
		bai->nr_free_blks_prepared + 
		bai->nr_clean_blks + 
		bai->nr_dirty_blks + 
		bai->nr_mgmt_blks + 
		bai->nr_bad_blks);
}

static inline
void __bdbm_abm_display_status (struct bdbm_abm_info* bai) 
{
	bdbm_msg ("[ABM] Total: %lu => Free:%lu, Free(prepare):%lu, Clean:%lu, Dirty:%lu, Bad:%lu",
		bai->nr_total_blks,
		bai->nr_free_blks, 
		bai->nr_free_blks_prepared, 
		bai->nr_clean_blks,
		bai->nr_dirty_blks,
		bai->nr_mgmt_blks,
		bai->nr_bad_blks);

	__bdbm_abm_check_status (bai);
}

babm_abm_subpage_t* __bdbm_abm_create_pst (struct nand_params* np)
{
	babm_abm_subpage_t* pst = NULL;

	/* NOTE: pst is managed in the unit of subpage to support fine-grain
	 * mapping FTLs that are often used to avoid expensive read-modify-writes
	 * */
	pst = bdbm_malloc (sizeof (babm_abm_subpage_t) * np->nr_subpages_per_block);
	bdbm_memset (pst, BABM_ABM_SUBPAGE_NOT_INVALID, sizeof (babm_abm_subpage_t) * np->nr_subpages_per_block);

	return pst;
};

void __bdbm_abm_destory_pst (babm_abm_subpage_t* pst) 
{
	if (pst)
		bdbm_free (pst);
}

struct bdbm_abm_info* bdbm_abm_create (
	struct nand_params* np,
	uint8_t use_pst)
{
	uint64_t loop;
	struct bdbm_abm_info* bai = NULL;

	/* create 'bdbm_abm_info' */
	if ((bai = (struct bdbm_abm_info*)bdbm_zmalloc (sizeof (struct bdbm_abm_info))) == NULL) {
		bdbm_error ("bdbm_zmalloc fbailed");
		return NULL;
	}
	bai->np = np;

	/* create 'bdbm_abm_block' */
	if ((bai->blocks = bdbm_zmalloc 
			(sizeof (struct bdbm_abm_block_t) * np->nr_blocks_per_ssd)) == NULL) {
		goto fail;
	}

	/* initialize 'bdbm_abm_block' */
	for (loop = 0; loop < np->nr_blocks_per_ssd; loop++) {
		bai->blocks[loop].status = BDBM_ABM_BLK_FREE;
		bai->blocks[loop].channel_no = __get_channel_ofs (np, loop);
		bai->blocks[loop].chip_no = __get_chip_ofs (np, loop);
		bai->blocks[loop].block_no = __get_block_ofs (np, loop);
		bai->blocks[loop].erase_count = 0;
		bai->blocks[loop].pst = NULL;
		bai->blocks[loop].nr_invalid_subpages = 0;
		/* create a 'page status table' (pst) if necessary */
		if (use_pst) {
			if ((bai->blocks[loop].pst = __bdbm_abm_create_pst (np)) == NULL) {
				bdbm_error ("__bdbm_abm_create_pst failed");
				goto fail;
			}
		}
	}

	/* build linked-lists */
	bai->list_head_free = (struct list_head**)bdbm_zmalloc (sizeof (struct list_head*) * np->nr_channels);
	bai->list_head_clean = (struct list_head**)bdbm_zmalloc (sizeof (struct list_head*) * np->nr_channels);
	bai->list_head_dirty = (struct list_head**)bdbm_zmalloc (sizeof (struct list_head*) * np->nr_channels);
	bai->list_head_mgmt = (struct list_head**)bdbm_zmalloc (sizeof (struct list_head*) * np->nr_channels);
	bai->list_head_bad = (struct list_head**)bdbm_zmalloc (sizeof (struct list_head*) * np->nr_channels);
	if (bai->list_head_free == NULL || 
		bai->list_head_clean == NULL || 
		bai->list_head_dirty == NULL || 
		bai->list_head_mgmt == NULL || 
		bai->list_head_bad == NULL) {
		bdbm_error ("bdbm_zmalloc failed");
		goto fail;
	}

	for (loop = 0; loop < np->nr_channels; loop++) {
		uint64_t subloop = 0;
		bai->list_head_free[loop] = (struct list_head*)bdbm_zmalloc 
			(sizeof (struct list_head) * np->nr_chips_per_channel);
		bai->list_head_clean[loop] = (struct list_head*)bdbm_zmalloc 
			(sizeof (struct list_head) * np->nr_chips_per_channel);
		bai->list_head_dirty[loop] = (struct list_head*)bdbm_zmalloc 
			(sizeof (struct list_head) * np->nr_chips_per_channel);
		bai->list_head_mgmt[loop] = (struct list_head*)bdbm_zmalloc 
			(sizeof (struct list_head) * np->nr_chips_per_channel);
		bai->list_head_bad[loop] = (struct list_head*)bdbm_zmalloc 
			(sizeof (struct list_head) * np->nr_chips_per_channel);

		if (bai->list_head_free[loop] == NULL || 
			bai->list_head_clean[loop] == NULL || 
			bai->list_head_dirty[loop] == NULL ||
			bai->list_head_mgmt[loop] == NULL ||
			bai->list_head_bad[loop] == NULL) {
			bdbm_error ("bdbm_zmalloc failed");
			goto fail;
		}
		for (subloop = 0; subloop < np->nr_chips_per_channel; subloop++) {
			INIT_LIST_HEAD (&bai->list_head_free[loop][subloop]);
			INIT_LIST_HEAD (&bai->list_head_clean[loop][subloop]);
			INIT_LIST_HEAD (&bai->list_head_dirty[loop][subloop]);
			INIT_LIST_HEAD (&bai->list_head_mgmt[loop][subloop]);
			INIT_LIST_HEAD (&bai->list_head_bad[loop][subloop]);
		}
	}

	/* add abm blocks into corresponding lists */
	for (loop = 0; loop < np->nr_blocks_per_ssd; loop++) {
		list_add_tail (&(bai->blocks[loop].list), 
			&(bai->list_head_free[bai->blocks[loop].channel_no][bai->blocks[loop].chip_no]));
	}

	/* initialize # of blocks according to their types */
	bai->nr_total_blks = np->nr_blocks_per_ssd;
	bai->nr_free_blks = bai->nr_total_blks;
	bai->nr_free_blks_prepared = 0;
	bai->nr_clean_blks = 0;
	bai->nr_dirty_blks = 0;
	bai->nr_mgmt_blks = 0;
	bai->nr_bad_blks = 0;

	bai->punits_iobb_pool = bdbm_malloc(sizeof(uint16_t) * np->nr_chips_per_ssd);
	for (loop = 0; loop < np->nr_chips_per_ssd; loop++) {
		bai->punits_iobb_pool[loop] = np->nr_blocks_per_chip;
	}

	/* done */
	return bai;

fail:
	bdbm_abm_destroy (bai);

	return NULL;
}

void bdbm_abm_destroy (struct bdbm_abm_info* bai) 
{
	uint64_t loop;

	if (bai == NULL)
		return;

	if (bai->list_head_free != NULL) {
		for (loop = 0; loop < bai->np->nr_channels; loop++)
			bdbm_free (bai->list_head_free[loop]);
		bdbm_free (bai->list_head_free);
	}
	if (bai->list_head_clean != NULL) {
		for (loop = 0; loop < bai->np->nr_channels; loop++)
			bdbm_free (bai->list_head_clean[loop]);
		bdbm_free (bai->list_head_clean);
	}
	if (bai->list_head_dirty != NULL) {
		for (loop = 0; loop < bai->np->nr_channels; loop++)
			bdbm_free (bai->list_head_dirty[loop]);
		bdbm_free (bai->list_head_dirty);
	}
	if (bai->list_head_bad != NULL) {
		for (loop = 0; loop < bai->np->nr_channels; loop++)
			bdbm_free (bai->list_head_bad[loop]);
		bdbm_free (bai->list_head_bad);
	}
	if (bai->blocks != NULL) {
		for (loop = 0; loop < bai->np->nr_blocks_per_ssd; loop++)
			__bdbm_abm_destory_pst (bai->blocks[loop].pst);
		bdbm_free (bai->blocks);
	}
	bdbm_free (bai);
}

/* get a block using an index */
struct bdbm_abm_block_t* bdbm_abm_get_block (
	struct bdbm_abm_info* bai,
	uint64_t channel_no,
	uint64_t chip_no,
	uint64_t block_no) 
{
	uint64_t blk_idx = 
		__get_block_idx (bai->np, channel_no, chip_no, block_no);

	/* see if blk_idx is correct or not */
	if (blk_idx >= bai->np->nr_blocks_per_ssd) {
		bdbm_error ("oops! blk_idx (%lu) is larger than # of blocks in SSD (%lu)",
			blk_idx, bai->np->nr_blocks_per_ssd);
		return NULL;
	}

	/* return */
	return &bai->blocks[blk_idx];
}

/* get a free block using lists */
struct bdbm_abm_block_t* bdbm_abm_get_free_block_prepare (
	struct bdbm_abm_info* bai,
	uint64_t channel_no,
	uint64_t chip_no) 
{
	struct list_head* pos = NULL;
	struct bdbm_abm_block_t* blk = NULL;
	uint32_t cnt = 0;
	uint64_t punit_id;
	punit_id = (channel_no * bai->np->nr_chips_per_channel) + chip_no;

	if (bai->nr_free_blks == 0) {
		bdbm_msg ("oops! bai->nr_free_blks == 0");
	}
	list_for_each (pos, &(bai->list_head_free[channel_no][chip_no])) {
		cnt++;
		blk = list_entry (pos, struct bdbm_abm_block_t, list);
		/*
		if (blk->block_no >= bai->punits_iobb_pool[punit_id]) {
			blk = NULL;
			continue;
		}
		*/
		if (blk->status == BDBM_ABM_BLK_FREE) {
			blk->status = BDBM_ABM_BLK_FREE_PREPARE;

			/* check some error cases */
			__bdbm_abm_check_status (bai);

			/* change the number of blks */
			bai->nr_free_blks--;
			bai->nr_free_blks_prepared++;
			break;
		}
		/* ignore if the status of a block is 'BDBM_ABM_BLK_FREE_PREPARE' */
		if (blk->status == BDBM_ABM_BLK_CLEAN) {
			bdbm_msg ("oops! blk->status == BDBM_ABM_BLK_CLEAN");
		}
	}

	
	if (blk == NULL) {
		bdbm_msg ("what???? blk == NULL: cnt = %u", cnt);
	}
	

	return blk;
}

void bdbm_abm_get_free_block_rollback (
	struct bdbm_abm_info* bai,
	struct bdbm_abm_block_t* blk)
{
	bdbm_bug_on (blk->status != BDBM_ABM_BLK_FREE_PREPARE);

	/* change the status of a block */
	blk->status = BDBM_ABM_BLK_FREE;

	/* check some error cases */
	bdbm_bug_on (bai->nr_free_blks_prepared == 0);
	__bdbm_abm_check_status (bai);

	/* change the number of blks */
	bai->nr_free_blks_prepared--;
	bai->nr_free_blks++;
}

void bdbm_abm_get_free_block_commit (
	struct bdbm_abm_info* bai,
	struct bdbm_abm_block_t* blk)
{
	/* see if the status of a block is correct or not */
	bdbm_bug_on (blk->status != BDBM_ABM_BLK_FREE_PREPARE);

	/* change the status */
	blk->status = BDBM_ABM_BLK_CLEAN;

	/* move it to 'clean_list' */
	list_del (&blk->list);
	list_add_tail (&blk->list, &(bai->list_head_clean[blk->channel_no][blk->chip_no]));

	/* check some error cases */
	bdbm_bug_on (bai->nr_free_blks_prepared == 0);
	__bdbm_abm_check_status (bai);

	/* change the number of blks */
	bai->nr_free_blks_prepared--;
	bai->nr_clean_blks++;
}

void bdbm_abm_erase_block (
	struct bdbm_abm_info* bai,
	uint64_t channel_no,
	uint64_t chip_no,
	uint64_t block_no,
	uint8_t is_bad)
{
	struct bdbm_abm_block_t* blk = NULL;
	uint64_t blk_idx = 
		__get_block_idx (bai->np, channel_no, chip_no, block_no);

	/* see if blk_idx is correct or not */
	if (blk_idx >= bai->np->nr_blocks_per_ssd) {
		bdbm_msg ("%lu %lu %lu", channel_no, chip_no, block_no);
		bdbm_error ("blk_idx (%lu) is larger than # of blocks in SSD (%lu)",
			blk_idx, bai->np->nr_blocks_per_ssd);
		return;
	} else {
		blk = &bai->blocks[blk_idx];
	}

	if (blk->channel_no != channel_no || blk->chip_no != chip_no || blk->block_no != block_no) {
		bdbm_error ("wrong block is chosen (%lu,%lu,%lu) != (%lu,%lu,%lu)",
			blk->channel_no, blk->chip_no, blk->block_no,
			channel_no, chip_no, block_no);
		return;
	}

	/*
	if (blk->status != BDBM_ABM_BLK_CLEAN &&
		blk->status != BDBM_ABM_BLK_DIRTY) {
		bdbm_warning ("blk->status is NOT BDBM_ABM_BLK_CLEAN or BDBM_ABM_BLK_DIRTY");
	}
	*/

	/* check some error cases */
	__bdbm_abm_check_status (bai);

	/* change # of blks */
	if (blk->status == BDBM_ABM_BLK_CLEAN) {
		bdbm_bug_on (bai->nr_clean_blks == 0);
		bai->nr_clean_blks--;
	} else if (blk->status == BDBM_ABM_BLK_DIRTY) {
		bdbm_bug_on (bai->nr_dirty_blks == 0);
		bai->nr_dirty_blks--;
	} else if (blk->status == BDBM_ABM_BLK_FREE) {
		bdbm_bug_on (bai->nr_free_blks == 0);
		bai->nr_free_blks--;
	} else if (blk->status == BDBM_ABM_BLK_FREE_PREPARE) {
		bdbm_bug_on (bai->nr_free_blks_prepared == 0);
		bai->nr_free_blks_prepared--;
	} else if (blk->status == BDBM_ABM_BLK_BAD) {
		bdbm_bug_on (bai->nr_bad_blks == 0);
		bai->nr_bad_blks--;
	} else if (blk->status == BDBM_ABM_BLK_MGMT) {
		bdbm_bug_on (bai->nr_mgmt_blks == 0);
		bai->nr_mgmt_blks--;
	} else {
		bdbm_bug_on (1);
	}

	if (is_bad == 1) {
		/* move it to 'bad_list' */
		list_del (&blk->list);
		list_add_tail (&blk->list, &(bai->list_head_bad[blk->channel_no][blk->chip_no]));
		bai->nr_bad_blks++;
		blk->status = BDBM_ABM_BLK_BAD;	/* mark it bad */

		bdbm_msg ("[BAD-BLOCK - MARKED] b:%lu c:%lu b:%lu p/e:%u", 
			blk->channel_no, 
			blk->chip_no, 
			blk->block_no, 
			blk->erase_count);

		/*__bdbm_abm_display_status (bai);*/
		__bdbm_abm_check_status (bai);

	} else {
		/* move it to 'free_list' */
		list_del (&blk->list);
		list_add_tail (&blk->list, &(bai->list_head_free[blk->channel_no][blk->chip_no]));
		bai->nr_free_blks++;
		blk->status = BDBM_ABM_BLK_FREE;
	}

	__bdbm_abm_check_status (bai);

	/* reset the block */
	blk->erase_count++;
	blk->nr_invalid_subpages = 0;
	if (blk->pst) {
		bdbm_memset (blk->pst, 
			BABM_ABM_SUBPAGE_NOT_INVALID, 
			sizeof (babm_abm_subpage_t) * bai->np->nr_subpages_per_block);
	}
}

void bdbm_abm_invalidate_page (
	struct bdbm_abm_info* bai, 
	uint64_t channel_no, 
	uint64_t chip_no, 
	uint64_t block_no, 
	uint64_t page_no,
	uint64_t subpage_no)
{
	struct bdbm_abm_block_t* b = NULL;
	uint64_t pst_off = 0;

	b = bdbm_abm_get_block (bai, channel_no, chip_no, block_no);

	bdbm_bug_on (b == NULL);
	bdbm_bug_on (page_no >= bai->np->nr_pages_per_block);
	bdbm_bug_on (b->channel_no != channel_no);
	bdbm_bug_on (b->chip_no != chip_no);
	bdbm_bug_on (b->block_no != block_no);
	bdbm_bug_on (subpage_no >= bai->np->nr_subpages_per_page);

	/* get a subpage offst in pst */
 	pst_off = (page_no * bai->np->nr_subpages_per_page) + subpage_no;
	bdbm_bug_on (pst_off >= bai->np->nr_subpages_per_block);

	/* if pst is NULL, ignore it */
	if (b->pst == NULL)
		return;

	if (b->pst[pst_off] == BABM_ABM_SUBPAGE_NOT_INVALID) {
		b->pst[pst_off] = BDBM_ABM_SUBPAGE_INVALID;
		/* is the block clean? */
		if (b->nr_invalid_subpages == 0) {
			if (b->status != BDBM_ABM_BLK_CLEAN) {
				bdbm_msg ("b->status: %u (%lu %lu %lu) (%lu %lu)", b->status, channel_no, chip_no, block_no, page_no, subpage_no);
				bdbm_bug_on (b->status != BDBM_ABM_BLK_CLEAN);
			}

			/* if so, its status is changed and then moved to a dirty list */
			b->status = BDBM_ABM_BLK_DIRTY;
			list_del (&b->list);
			list_add_tail (&b->list, &(bai->list_head_dirty[b->channel_no][b->chip_no]));

			bdbm_bug_on (bai->nr_clean_blks == 0);
			__bdbm_abm_check_status (bai);

			bai->nr_clean_blks--;
			bai->nr_dirty_blks++;
		}
		/* increase # of invalid pages in the block */
		b->nr_invalid_subpages++;
		bdbm_bug_on (b->nr_invalid_subpages > bai->np->nr_subpages_per_block);
	} else {
		/* ignore if it was invalidated before */
	}
}


/* for snapshot */
uint32_t bdbm_abm_load (struct bdbm_abm_info* bai, const char* fn)
{
#if 0
	struct file* fp = NULL;
	uint64_t i, pos = 0;

	if ((fp = bdbm_fopen (fn, O_RDWR, 0777)) == NULL) {
		bdbm_error ("bdbm_fopen failed");
		return 1;
	}

	/* step1: load a set of bdbm_abm_block_t */
	for (i = 0; i < bai->np->nr_blocks_per_ssd; i++) {
		pos += bdbm_fread (fp, pos, (uint8_t*)&bai->blocks[i].status, sizeof(bai->blocks[i].status));
		pos += bdbm_fread (fp, pos, (uint8_t*)&bai->blocks[i].channel_no, sizeof(bai->blocks[i].channel_no));
		pos += bdbm_fread (fp, pos, (uint8_t*)&bai->blocks[i].chip_no, sizeof(bai->blocks[i].chip_no));
		pos += bdbm_fread (fp, pos, (uint8_t*)&bai->blocks[i].block_no, sizeof(bai->blocks[i].block_no));
		pos += bdbm_fread (fp, pos, (uint8_t*)&bai->blocks[i].erase_count, sizeof(bai->blocks[i].erase_count));
		pos += bdbm_fread (fp, pos, (uint8_t*)&bai->blocks[i].nr_invalid_subpages, sizeof(bai->blocks[i].nr_invalid_subpages));
		if (bai->blocks[i].pst) {
			pos += bdbm_fread (fp, pos, (uint8_t*)bai->blocks[i].pst, sizeof (babm_abm_subpage_t) * bai->np->nr_subpages_per_block);
		} else {
			pos += sizeof (babm_abm_subpage_t) * bai->np->nr_subpages_per_block;
		}
	}
#endif
	/* step1: load a set of bdbm_abm_block_t */
	FILE *fp=NULL;
	uint64_t i;
	long pos;
	if ((fp=fopen(fn,"r")) == NULL) {
		//bdbm_error ("bdbm_fopen failed");
		syslog (LOG_INFO,"<<<Not found abm.dat>>>\n");
		return 1;
	}
	syslog (LOG_INFO,"Reading the abm.dat.........\n");
	for (i = 0; i < bai->np->nr_blocks_per_ssd; i++) {
		fread ((uint8_t*)&bai->blocks[i].status, 1, sizeof(bai->blocks[i].status) ,fp);
		fread ((uint8_t*)&bai->blocks[i].channel_no, 1, sizeof(bai->blocks[i].channel_no), fp);
		fread ((uint8_t*)&bai->blocks[i].chip_no, 1, sizeof(bai->blocks[i].chip_no), fp);
		fread ((uint8_t*)&bai->blocks[i].block_no, 1, sizeof(bai->blocks[i].block_no), fp);
		fread ((uint8_t*)&bai->blocks[i].erase_count, 1, sizeof(bai->blocks[i].erase_count), fp);
		fread ((uint8_t*)&bai->blocks[i].nr_invalid_subpages, 1, sizeof(bai->blocks[i].nr_invalid_subpages), fp);
		if (bai->blocks[i].pst) {
			fread ((uint8_t*)bai->blocks[i].pst, 1, sizeof (babm_abm_subpage_t) * bai->np->nr_subpages_per_block, fp);
		} else { 
			pos = sizeof (babm_abm_subpage_t) * bai->np->nr_subpages_per_block;
			fseek(fp, pos, SEEK_CUR);
		}
	}


	/* step2: build lists & # of blocks */
	bai->nr_free_blks = 0;
	bai->nr_free_blks_prepared = 0;
	bai->nr_clean_blks = 0;
	bai->nr_dirty_blks = 0;
	bai->nr_mgmt_blks = 0;
	bai->nr_bad_blks = 0;

	for (i = 0; i < bai->np->nr_blocks_per_ssd; i++) {
		struct bdbm_abm_block_t* b = &bai->blocks[i];
		list_del (&b->list);
		switch (b->status) {
			case BDBM_ABM_BLK_FREE:
				list_add_tail (&b->list, &(bai->list_head_free[b->channel_no][b->chip_no]));
				bai->nr_free_blks++;
				break;
			case BDBM_ABM_BLK_FREE_PREPARE:
				list_add_tail (&b->list, &(bai->list_head_free[b->channel_no][b->chip_no]));
				bai->nr_free_blks_prepared++;
				break;
			case BDBM_ABM_BLK_CLEAN:
				list_add_tail (&b->list, &(bai->list_head_clean[b->channel_no][b->chip_no]));
				bai->nr_clean_blks++;
				break;
			case BDBM_ABM_BLK_DIRTY:
				list_add_tail (&b->list, &(bai->list_head_dirty[b->channel_no][b->chip_no]));
				bai->nr_dirty_blks++;
				break;
			case BDBM_ABM_BLK_MGMT:
				list_add_tail (&b->list, &(bai->list_head_mgmt[b->channel_no][b->chip_no]));
				bai->nr_mgmt_blks++;
				break;

			case BDBM_ABM_BLK_BAD:
				list_add_tail (&b->list, &(bai->list_head_bad[b->channel_no][b->chip_no]));
				bai->nr_bad_blks++;
				break;
			default:
				bdbm_error ("invalid block type: blk-id = %lu, blk-status = %u", i, b->status);
				break;
		}
	}

	/* step3: display */
	syslog (LOG_INFO,"abm-load: free:%lu, free(prepare):%lu, clean:%lu, dirty:%lu, mgmt:%lu bad:%lu",
			bai->nr_free_blks, 
			bai->nr_free_blks_prepared, 
			bai->nr_clean_blks,
			bai->nr_dirty_blks,
			bai->nr_mgmt_blks,
			bai->nr_bad_blks);
	fclose(fp);
	//bdbm_fclose (fp);

	return 0;
}

uint32_t bdbm_abm_store (struct bdbm_abm_info* bai, const char* fn)
{
#if 0
	struct file* fp = NULL;
	uint64_t i, pos = 0;

	if ((fp = bdbm_fopen (fn,  O_CREAT | O_WRONLY, 0777)) == NULL) {
		bdbm_error ("bdbm_fopen failed");
		return 1;
	}

	/* step1: store a set of bdbm_abm_block_t */
	for (i = 0; i < bai->np->nr_blocks_per_ssd; i++) {
		pos += bdbm_fwrite (fp, pos, (uint8_t*)&bai->blocks[i].status, sizeof(bai->blocks[i].status));
		pos += bdbm_fwrite (fp, pos, (uint8_t*)&bai->blocks[i].channel_no, sizeof(bai->blocks[i].channel_no));
		pos += bdbm_fwrite (fp, pos, (uint8_t*)&bai->blocks[i].chip_no, sizeof(bai->blocks[i].chip_no));
		pos += bdbm_fwrite (fp, pos, (uint8_t*)&bai->blocks[i].block_no, sizeof(bai->blocks[i].block_no));
		pos += bdbm_fwrite (fp, pos, (uint8_t*)&bai->blocks[i].erase_count, sizeof(bai->blocks[i].erase_count));
		pos += bdbm_fwrite (fp, pos, (uint8_t*)&bai->blocks[i].nr_invalid_subpages, sizeof(bai->blocks[i].nr_invalid_subpages));
		if (bai->blocks[i].pst) {
			pos += bdbm_fwrite (fp, pos, (uint8_t*)bai->blocks[i].pst, sizeof (babm_abm_subpage_t) * bai->np->nr_subpages_per_block);
		} else {
			pos += sizeof (babm_abm_subpage_t) * bai->np->nr_subpages_per_block;
		}
	}
#endif
	uint64_t i;
	long pos;
	FILE *fp = NULL;
	if ((fp=fopen(fn,"w")) == NULL) {
		bdbm_error ("bdbm_fopen failed");
		return 1;
	}
	syslog (LOG_INFO,"Writing to abm.dat..............\n");	
	for (i = 0; i < bai->np->nr_blocks_per_ssd; i++) {
		fwrite((uint8_t*)&bai->blocks[i].status, 1, sizeof(bai->blocks[i].status), fp);
		fwrite((uint8_t*)&bai->blocks[i].channel_no, 1, sizeof(bai->blocks[i].channel_no), fp);
		fwrite((uint8_t*)&bai->blocks[i].chip_no, 1, sizeof(bai->blocks[i].chip_no), fp);
		fwrite((uint8_t*)&bai->blocks[i].block_no, 1, sizeof(bai->blocks[i].block_no), fp);
		fwrite((uint8_t*)&bai->blocks[i].erase_count, 1, sizeof(bai->blocks[i].erase_count), fp);
		fwrite((uint8_t*)&bai->blocks[i].nr_invalid_subpages, 1, sizeof(bai->blocks[i].nr_invalid_subpages), fp);
		if (bai->blocks[i].pst) {
			fwrite((uint8_t*)bai->blocks[i].pst, 1, sizeof (babm_abm_subpage_t) * bai->np->nr_subpages_per_block, fp);
		} else { 
			pos = sizeof (babm_abm_subpage_t) * bai->np->nr_subpages_per_block;
			fseek(fp, pos, SEEK_CUR);
		}
	}

	syslog (LOG_INFO,"abm-store: free:%lu, free(prepare):%lu, clean:%lu, dirty:%lu, mgmt:%lu bad:%lu",
			bai->nr_free_blks, 
			bai->nr_free_blks_prepared, 
			bai->nr_clean_blks,
			bai->nr_dirty_blks,
			bai->nr_mgmt_blks,
			bai->nr_bad_blks);

	//bdbm_fsync (fp);
	//bdbm_fclose (fp);
	fclose(fp);
	return 0;
}

