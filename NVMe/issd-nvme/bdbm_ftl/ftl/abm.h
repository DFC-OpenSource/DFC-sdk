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

#ifndef _BLUEDBM_FTL_ABM_H
#define _BLUEDBM_FTL_ABM_H
/*
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/list.h>
*/
#include "../bdbm_drv.h"
#include "../params.h"
#include "../list.h"

enum BDBM_ABM_SUBPAGE_STATUS {
	BABM_ABM_SUBPAGE_NOT_INVALID = 0,
	BDBM_ABM_SUBPAGE_INVALID,
};

typedef uint8_t babm_abm_subpage_t; /* BDBM_ABM_PAGE_STATUS */

enum BDBM_ABM_BLK_STATUS {
	BDBM_ABM_BLK_FREE = 0,
	BDBM_ABM_BLK_FREE_PREPARE,
	BDBM_ABM_BLK_CLEAN,
	BDBM_ABM_BLK_DIRTY,

	BDBM_ABM_BLK_MGMT,
	BDBM_ABM_BLK_BAD,
};

struct bdbm_abm_block_t {
	uint8_t status;	/* ABM_BLK_STATUS */
	uint8_t channel_no;
	uint8_t chip_no;
	uint16_t block_no;
	uint32_t erase_count;
	uint16_t nr_invalid_subpages;
	babm_abm_subpage_t* pst;	/* a page status table; used when the FTL requires */

	struct list_head list;	/* for list */
};

struct bdbm_abm_info {
	struct nand_params* np;
	struct bdbm_abm_block_t* blocks;
	struct list_head** list_head_free;
	struct list_head** list_head_clean;
	struct list_head** list_head_dirty;
	struct list_head** list_head_mgmt;
	struct list_head** list_head_bad;

	/* # of blocks according to their types */
	uint64_t nr_total_blks;
	uint64_t nr_free_blks;
	uint64_t nr_free_blks_prepared;
	uint64_t nr_clean_blks;
	uint64_t nr_dirty_blks;
	uint64_t nr_mgmt_blks;
	uint64_t nr_bad_blks;

	uint16_t *punits_iobb_pool;   /*blocks boundary per chip*/
};

struct bdbm_abm_info* bdbm_abm_create (struct nand_params* np, uint8_t use_pst);
void bdbm_abm_destroy (struct bdbm_abm_info* bai);
struct bdbm_abm_block_t* bdbm_abm_get_block (struct bdbm_abm_info* bai, uint64_t channel_no, uint64_t chip_no, uint64_t block_no);
struct bdbm_abm_block_t* bdbm_abm_get_free_block_prepare (struct bdbm_abm_info* bai, uint64_t channel_no, uint64_t chip_no);
void bdbm_abm_get_free_block_rollback (struct bdbm_abm_info* bai, struct bdbm_abm_block_t* blk);
void bdbm_abm_get_free_block_commit (struct bdbm_abm_info* bai, struct bdbm_abm_block_t* blk);
void bdbm_abm_erase_block (struct bdbm_abm_info* bai, uint64_t channel_no, uint64_t chip_no, uint64_t block_no, uint8_t is_bad);
void bdbm_abm_invalidate_page (struct bdbm_abm_info* bai, uint64_t channel_no, uint64_t chip_no, uint64_t block_no, uint64_t page_no, uint64_t subpage_no);

static inline uint64_t bdbm_abm_get_nr_free_blocks (struct bdbm_abm_info* bai) { return bai->nr_free_blks; }
static inline uint64_t bdbm_abm_get_nr_free_blocks_prepared (struct bdbm_abm_info* bai) { return bai->nr_free_blks_prepared; }
static inline uint64_t bdbm_abm_get_nr_clean_blocks (struct bdbm_abm_info* bai) { return bai->nr_clean_blks; }
static inline uint64_t bdbm_abm_get_nr_dirty_blocks (struct bdbm_abm_info* bai) { return bai->nr_dirty_blks; }
static inline uint64_t bdbm_abm_get_nr_total_blocks (struct bdbm_abm_info* bai) { return bai->nr_total_blks; }

uint32_t bdbm_abm_load (struct bdbm_abm_info* bai, const char* fn);
uint32_t bdbm_abm_store (struct bdbm_abm_info* bai, const char* fn);

#define bdbm_abm_list_for_each_dirty_block(pos, bai, channel_no, chip_no) \
	list_for_each (pos, &(bai->list_head_dirty[channel_no][chip_no]))
#define bdbm_abm_fetch_dirty_block(pos) \
	list_entry (pos, struct bdbm_abm_block_t, list)
/*  (example:)
 *  bdbm_abm_list_for_each_dirty_block (pos, p->bai, j, k) {
		b = bdbm_abm_fetch_dirty_block (pos);
	}
 */

#endif

