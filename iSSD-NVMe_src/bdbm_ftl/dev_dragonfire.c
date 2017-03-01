
#include <stdio.h>
#include <string.h>
#include <semaphore.h>
#include "../nvme.h"
#include "dev_dragonfire.h"
#include "platform.h"
#include "params.h"
#include "../dma_df_nand.h"
#include "../onfi_params.h"
#include "../uatomic.h"
#include <syslog.h>
extern bdbm_dev_private_t _dp;
extern NvmeCtrl g_NvmeCtrl;
extern atomic_t act_desc_cnt;

extern int bdbm_badge_dimms (bdbm_dev_private_t *pdp);

/****************************BDBM_LOAD**********************************/
static void bdbm_find_last_good_block_in_chip (bdbm_nandaddr_t *spare, local_mem_t *lmem)
{
	int ret = 0;
	sem_t waiter;

	sem_init (&waiter, 0, 0);

	while (spare->row.block_no != (uint64_t)(-1)) {
		df_setup_nand_rd_desc((void*)&waiter, &spare->row,  spare->col, \
				lmem->phy, _dp.nand.page_oob_size, DF_BITS_MGMT_OOB);
		//setup_nand_rd_desc (spare, lmem->phy, \
				pdp.nand.page_oob_size, &waiter, &ret);
		sem_wait (&waiter);
		if (*(lmem->virt) == 0xFF) { /* it's a good block */
			/* could be (or can be) a magic block */
			break;
		}
		spare->row.block_no--;
	}

	sem_destroy (&waiter);

	if (spare->row.block_no == (uint64_t)(-1)) {
		syslog (LOG_INFO,"no good block found: %lu-%lu-%lu-%lu-%u\n", \
				spare->row.channel_no, spare->row.chip_no, spare->row.block_no, \
				spare->row.page_no, spare->col);
		bdbm_bug_on (spare->row.block_no == (uint64_t)(-1));
	}
}

static void bdbm_collect_mgmt_blocks (bdbm_dev_private_t *pdp)
{
	int channelCnt = 0, chipCnt = 0, blockCnt = 0, mgmtBlkCnt = 0, punits = 0;
	bdbm_nandaddr_t block_addr = {{0}};
	bdbm_device_params_t *nand = &pdp->nand;
	bdbm_nblock_st_t *mgmtBlks;
	uint32_t nr_chips = nand->nr_chips_per_channel;
	uint32_t nr_channels = nand->nr_channels;
	local_mem_t lmem = {0};
	uint32_t nMgmtBlksPerChip = pdp->nr_mgmt_blocks_per_ssd / \
								pdp->nand.nr_chips_per_ssd;

	bdbm_alloc_pv (&lmem, nand->page_oob_size);

	block_addr.col = nand->page_main_size;

	syslog (LOG_INFO,"collecting %lu blocks for mgmt!\n",	pdp->nr_mgmt_blocks_per_ssd);
	for (channelCnt = 0; channelCnt < nr_channels; \
			channelCnt++, block_addr.row.channel_no++) {
		block_addr.row.chip_no = 0;
		for (chipCnt = 0; chipCnt < nr_chips; \
				chipCnt++, block_addr.row.chip_no++) {
			block_addr.row.block_no = nand->nr_blocks_per_chip - 1;
			mgmtBlks = pdp->mgmt_blocks[punits];
			mgmtBlkCnt = 0;
			for (blockCnt = 0; (blockCnt < nand->nr_blocks_per_chip) && \
					(mgmtBlkCnt < nMgmtBlksPerChip); \
					blockCnt++, mgmtBlkCnt++) {
				bdbm_find_last_good_block_in_chip (&block_addr, &lmem);
				mgmtBlks[mgmtBlkCnt].channel = block_addr.row.channel_no;
				mgmtBlks[mgmtBlkCnt].chip = block_addr.row.chip_no;
				mgmtBlks[mgmtBlkCnt].block = block_addr.row.block_no;
				mgmtBlks[mgmtBlkCnt].erase_count = 0;
				mgmtBlks[mgmtBlkCnt].nr_invalid_subpages = 0;
				mgmtBlks[mgmtBlkCnt].status = BDBM_ABM_BLK_FREE;
				syslog (LOG_INFO,"mb: %u-%u %d-%d on chip %d\n", \
						mgmtBlks[mgmtBlkCnt].channel, mgmtBlks[mgmtBlkCnt].block, \
						mgmtBlkCnt, blockCnt, chipCnt);
				block_addr.row.block_no--;
			}
			if (blockCnt == NR_BLOCKS_PER_CHIP) {
				syslog (LOG_INFO,"couldn't get mgmt blocks from chip %u\n", chipCnt);
				bdbm_bug_on (1);
			}
			punits++;
		}
	}
}

static void bdbm_setup_pm_mgmt_blocks (bdbm_dev_private_t *pdp)
{
	int blkIdx, chipIdx, blkCnt = 0;
	bdbm_nblock_st_t *pmtb_pool = pdp->bmi[MGMTB_PMTB_POOL];
	uint32_t nr_chips = pdp->nand.nr_chips_per_ssd;
	bdbm_nblock_st_t **mgmtBlks = pdp->mgmt_blocks;
	uint32_t nBlks_PmtBPool = pdp->mgmt_blkptr[MGMTB_PMTB_POOL].nEntries;
	uint32_t nr_dimms = pdp->dimms.nr_dimms;
	uint32_t nr_mgmt_blks_per_chip = pdp->nr_mgmt_blocks_per_ssd/nr_chips;

	/* put all pmt mgmt blocks in pmtb_pool pool forming a pattern
	 * for levelled distribution explained as follows: 
	 * blk_N from all chips come first, blk_N+1 next
	 * taking care of magic block from the dimm(s). 
	 * */
	for (chipIdx = 0; chipIdx < nr_chips; chipIdx++) {
		if (chipIdx % (nr_chips / nr_dimms)) {
			memcpy (&pmtb_pool[blkCnt], \
					&mgmtBlks[chipIdx][0], sizeof (bdbm_nblock_st_t));
			syslog (LOG_INFO,"%d: %u-%u\n", blkCnt, \
					pmtb_pool[blkCnt].channel, pmtb_pool[blkCnt].block);
			blkCnt++;
		}
	}
	syslog (LOG_INFO,"collecting from non-magicable blocks\n");
	for (blkIdx = 1; blkIdx < nr_mgmt_blks_per_chip; blkIdx++) {
		for (chipIdx = 0; chipIdx < nr_chips; chipIdx++, blkCnt++) {
			memcpy (&pmtb_pool[blkCnt], \
					&mgmtBlks[chipIdx][blkIdx], sizeof (bdbm_nblock_st_t));
			syslog (LOG_INFO,"%d: %u-%u\n", blkCnt, \
					pmtb_pool[blkCnt].channel, pmtb_pool[blkCnt].block);
		}
	}
}

static void bdbm_init_nand_params (bdbm_device_params_t *np, uint8_t  *virt)
{
	ONFI_Param_Pg op;
	memcpy(&op,virt,sizeof(ONFI_Param_Pg));
	np->nr_chips_per_channel = NR_CHIPS_PER_CHANNEL;/*TODO need to update based on target and LUN count*/
	np->nr_blocks_per_chip 	 = op.onfi_mem_org.blks_per_lun;
	np->nr_pages_per_block 	 = op.onfi_mem_org.pgs_per_blk;
	np->page_main_size 		 = op.onfi_mem_org.databytes_per_pg;
	np->page_oob_size 		 = op.onfi_mem_org.sparebytes_per_pg;
	np->page_oob_size		 = NAND_PAGE_OOB_SIZE;/*TODO need to remove after 4-1 mapping enabled*/
	np->subpage_size 		 = KERNEL_PAGE_SIZE;
	np->page_prog_time_us 	 = op.onfi_elec_param.t_prog;
	np->page_read_time_us 	 = op.onfi_elec_param.t_r;
	np->block_erase_time_us  = op.onfi_elec_param.t_bers;

	np->nr_blocks_per_channel = 
		np->nr_chips_per_channel *
		np->nr_blocks_per_chip;
	np->nr_blocks_per_ssd = 
		np->nr_channels * np->nr_blocks_per_channel;
	np->nr_chips_per_ssd =
		np->nr_channels * np->nr_chips_per_channel;
	np->nr_pages_per_ssd = 
		np->nr_pages_per_block *
		np->nr_blocks_per_ssd;

	np->nr_subpages_per_page  = np->page_main_size / np->subpage_size;
	np->nr_subpages_per_block = np->nr_subpages_per_page * np->nr_pages_per_block;
	np->nr_subpages_per_ssd   = np->nr_subpages_per_page * np->nr_pages_per_ssd;

	np->device_capacity_in_byte = 0;
	np->device_capacity_in_byte += np->nr_channels;
	np->device_capacity_in_byte *= np->nr_chips_per_channel;

	/*TODO considering as MAPPING_POLICY_PAGE*/
	np->device_capacity_in_byte *=
		(np->nr_blocks_per_chip - np->nr_blocks_per_chip * 0.06);/*for FTL overprivisioning */
	np->device_capacity_in_byte *= np->nr_pages_per_block;
	np->device_capacity_in_byte *= np->page_main_size;
}

static void bdbm_read_nand_spd (bdbm_device_params_t *np)
{
	int ret;
	local_mem_t lmem = {0};
	sem_t waiter;
	sem_init (&waiter, 0, 0);
	struct bdbm_phyaddr_t phy_addr = {0};
	
	bdbm_alloc_pv (&lmem, 256);
	ret = df_setup_nand_rd_desc ((void*)&waiter,&phy_addr,0,lmem.phy,256,DF_BIT_CONFIG_IO|DF_BIT_SEM_USED);
	sem_wait (&waiter); 

	bdbm_init_nand_params (np, lmem.virt);
}

static int bdbm_detect_dimms (bdbm_dimm_status_t *dimms, bdbm_device_params_t *np)
{
	int i;
	dimms->nr_dimms = 0;
	np->nr_channels = 0;

	for (i=0; i<2; i++) {
		if (g_NvmeCtrl.dm[i*2]->valid) {
			dimms->nr_dimms += 1;
			np->nr_channels += 4;
		}
	}

	if (dimms->nr_dimms != 0) {
		bdbm_read_nand_spd (np);
	}

	if (dimms->nr_dimms == 2) {
#include <time.h>
		time_t t;
		uint32_t a, b;

		time(&t);
		a = rand_r ((uint32_t *)&t);
		t += t;
		b = rand_r ((uint32_t *)&t);
		if (a > b) {
			a = a + b;
			b = a - b;
			a = a - b;
		}
		dimms->dimm_tags[0] = a;
		dimms->dimm_tags[1] = b;
	} else if (dimms->nr_dimms == 0) {
		return -1;
	}

	return 0;
}

void alloc_mgmt_block_arrays (bdbm_dev_private_t *pdp)
{
	int i;
	bdbm_nblock_st_t *pmtb_curr, *pmtb_pool, *iobb_pool;
	uint32_t nBlks_PmtBPool = pdp->mgmt_blkptr[MGMTB_PMTB_POOL].nEntries;
	uint32_t nBlks_PmtBCurr = pdp->mgmt_blkptr[MGMTB_PMTB_CURR].nEntries;
	uint32_t nBlks_iobbPool = pdp->mgmt_blkptr[MGMTB_IOBB_POOL].nEntries;
	uint32_t nMgmtBlksPerChip =  nBlks_PmtBPool + pdp->dimms.nr_dimms;

	pdp->mgmt_blocks = (bdbm_nblock_st_t **) bdbm_zmalloc (sizeof(bdbm_nblock_st_t*) * pdp->nand.nr_chips_per_ssd);
	for (i = 0; i < pdp->nand.nr_chips_per_ssd; i++) {
		pdp->mgmt_blocks[i] = (bdbm_nblock_st_t *) \
							  bdbm_zmalloc (nMgmtBlksPerChip * sizeof (bdbm_nblock_st_t));
	}

	pmtb_pool = bdbm_malloc (nBlks_PmtBPool * sizeof (bdbm_nblock_st_t));
	bdbm_bug_on (pmtb_pool == NULL);
	pmtb_curr = bdbm_malloc (nBlks_PmtBCurr * sizeof (bdbm_nblock_st_t));
	bdbm_bug_on (pmtb_curr == NULL);
	iobb_pool  = bdbm_malloc (nBlks_iobbPool * sizeof (bdbm_nblock_st_t));
	bdbm_bug_on (iobb_pool == NULL);

	pdp->bmi[MGMTB_PMTB_CURR] = pmtb_curr;
	pdp->bmi[MGMTB_PMTB_POOL] = pmtb_pool;
	pdp->bmi[MGMTB_IOBB_POOL]  = iobb_pool;
//	pdp->bmi[MGMTB_BMT] = (bdbm_nblock_st_t *)pdp->bai->blocks;
}

void update_nand_capacity (bdbm_dev_private_t *pdp)
{
	bdbm_device_params_t *nand = &pdp->nand;

	nand->device_capacity_in_byte = nand->nr_blocks_per_ssd - \
									pdp->nr_mgmt_blocks_per_ssd;
	nand->device_capacity_in_byte *= nand->nr_pages_per_block * \
									 nand->page_main_size;
}

void setup_def_mgmt_params (bdbm_dev_private_t *pdp)
{
	bdbm_device_params_t *nand = &pdp->nand;
	uint32_t nr_chips = nand->nr_chips_per_ssd;

	float fnBlksPmt = (float)(nand->nr_subpages_per_ssd * sizeof(bdbm_npme_t)) \
					  / nand->page_main_size / nand->nr_pages_per_block;
	uint32_t nBlks_PmtBCurr = (int)fnBlksPmt + \
							  !!((fnBlksPmt - (float)(int)fnBlksPmt) * 10);
	uint32_t nBlksPmt_pool = nBlks_PmtBCurr * 3 /* 3*size for pool */;
	uint32_t nBlksMgmt = nBlksPmt_pool + 2 /* magic blocks */;

	/* round the pool block count to multiple of nr_blocks_per_ssd */
	if (nBlksMgmt <= nr_chips) {
		nBlksMgmt = nr_chips;
	} else {
		nBlksMgmt = ((nBlksMgmt / nr_chips) + \
				!!(nBlksMgmt % nr_chips)) * nr_chips;
	}

	/* set the size now occupied by pm(fc)b */
	pdp->mgmt_blkptr[MGMTB_PMTB_POOL].nEntries = nBlksMgmt-pdp->dimms.nr_dimms;
	pdp->mgmt_blkptr[MGMTB_PMTB_CURR].nEntries = nBlks_PmtBCurr;
	pdp->mgmt_blkptr[MGMTB_BMT].nEntries = nand->nr_blocks_per_ssd;
	pdp->mgmt_blkptr[MGMTB_IOBB_POOL].nEntries = nand->nr_chips_per_ssd;


	pdp->nr_mgmt_blocks_per_ssd = nBlksMgmt;

	alloc_mgmt_block_arrays (pdp);

	update_nand_capacity (pdp);
}

static void setup_mgmt_block_ptrs (bdbm_dev_private_t *pdp)
{
	bdbm_nandaddr_t addr;
	uint32_t nFpages;
	uint32_t entriesPerFPage = pdp->nand.page_main_size / \
							   sizeof (bdbm_nblock_st_t);
	uint32_t nEntries;

	/* set the base addresses of areas alloted to pmtbs and abms */
	addr.row.channel_no = pdp->mgmt_blocks[0][0].channel;
	addr.row.chip_no = pdp->mgmt_blocks[0][0].chip;
	addr.row.block_no = pdp->mgmt_blocks[0][0].block;
	addr.col = 0;

	/* area: pmtb_pool */
	addr.row.page_no = MGMTB_MGMT_PTR_PAGE + 1;
	memcpy (&pdp->mgmt_blkptr[MGMTB_PMTB_CURR].addr, &addr, sizeof (addr));

	/* area: pmtb_curr */
	nEntries = pdp->mgmt_blkptr[MGMTB_PMTB_CURR].nEntries;
	nFpages = nEntries / entriesPerFPage + \
		   !!(nEntries % entriesPerFPage);
	addr.row.page_no += nFpages;
	memcpy (&pdp->mgmt_blkptr[MGMTB_PMTB_POOL].addr, &addr, sizeof (addr));

	/* area: abm */
	nEntries = pdp->mgmt_blkptr[MGMTB_PMTB_POOL].nEntries;
	nFpages = nEntries / entriesPerFPage + \
		   !!(nEntries % entriesPerFPage);
	addr.row.page_no += nFpages;
	memcpy (&pdp->mgmt_blkptr[MGMTB_BMT].addr, &addr, sizeof (addr));
	
	/* area: io block boundaries per chip */
	nEntries = pdp->mgmt_blkptr[MGMTB_BMT].nEntries;
	nFpages = nEntries / entriesPerFPage + \
		   !!(nEntries % entriesPerFPage);
	addr.row.page_no += nFpages;
	memcpy (&pdp->mgmt_blkptr[MGMTB_IOBB_POOL].addr, &addr, sizeof (addr)); 	
}

static void bdbm_setup_iobb_pool (bdbm_dev_private_t *pdp) 
{
	bdbm_nblock_st_t *iobb_pool;
	uint32_t mgmt_blk_per_chip = pdp->nr_mgmt_blocks_per_ssd/pdp->nand.nr_chips_per_ssd;
	uint8_t punits;

	iobb_pool = pdp->bmi[MGMTB_IOBB_POOL];
	for (punits = 0; punits < pdp->mgmt_blkptr[MGMTB_IOBB_POOL].nEntries; \
		punits++,iobb_pool++) {
		memcpy(iobb_pool, &pdp->mgmt_blocks[punits][mgmt_blk_per_chip-1], \
			sizeof(bdbm_nblock_st_t));
	}
}

static void bdbm_reset_ssd (bdbm_dev_private_t *pdp)
{
	uint64_t nr_chips = pdp->nand.nr_channels;
	uint64_t nr_target = 2;
	uint64_t chip, i = 0;
	uint32_t ret = 0;
	bdbm_phyaddr_t phy_addr = {0};
	sem_t waiter;

	sem_init (&waiter, 0, 0);

	for (chip = 0; chip < nr_chips; chip++) {
		phy_addr.channel_no = chip;
		for (i = 0; i < nr_target; i++) {
			phy_addr.chip_no = i * 2;
			ret = df_setup_nand_desc((void*)&waiter, &phy_addr, 0, NULL, \
					0, DF_BIT_MGMT_IO|DF_BIT_RESET_OP); 
			sem_wait (&waiter);
		}
	}
	sem_destroy (&waiter);
}

int setup_new_ssd_context (bdbm_dev_private_t *pdp)
{
	int ret = 0;

	if (pdp->dimm_status != BDBM_ST_EXIT_GOOD) {
		/* setup the mgmt block info */
		setup_def_mgmt_params (pdp);
		/* for good exit, all mapping info and corresponding pointers
		 * are stored in (first) magic block, so for other status,
		 * collect mgmt_blkptr blocks afresh */
		bdbm_collect_mgmt_blocks (pdp);
		/* assign locations for the mgmt_blk pointers */
		setup_mgmt_block_ptrs (pdp);
		/* assign badge(s) */
		bdbm_badge_dimms (pdp);
		/* now setup PMT Free and Used block lists */
		bdbm_setup_pm_mgmt_blocks (pdp);
		/*get the starting management block address from each chip*/
		bdbm_setup_iobb_pool (pdp);

		pdp->dimm_status = BDBM_ST_DIMMS_NEW;
	}

	return ret;
}

int bdbm_get_dimm_status (bdbm_dev_private_t *pdp)
{
	/* last block, 1st page spare */
	bdbm_nandaddr_t spare = {{0}}, spare_bk = {{0}};
	local_mem_t lmem = {0};
	int magic_done = 0;
	uint32_t dimm_tags[2][2] = {{0}};
	int ret = 0,i;
	sem_t waiter;
	
	bdbm_reset_ssd (pdp);

	ret = bdbm_detect_dimms (&pdp->dimms, &pdp->nand);
	bdbm_bug_on (ret < 0);

	sem_init (&waiter, 0, 0);

	pdp->dimm_status = BDBM_ST_DIMMS_FRESH;

	spare.row.block_no = pdp->nand.nr_blocks_per_chip - 1;
	spare.col = pdp->nand.page_main_size;

	bdbm_alloc_pv (&lmem, pdp->nand.page_oob_size);

NEXT_DIMM:
	bdbm_find_last_good_block_in_chip (&spare, &lmem);
	if (!magic_done) {
		/* first (and only?) dimm */
		spare_bk = spare;
		magic_done = (*(uint32_t *)lmem.virt == MAGIC_KEY)?1:0;
	} else {
		/* second dimm present */

		if (*(uint32_t *)lmem.virt != MAGIC_KEY) {
			/* it doesn't have magic key, print and return error */
			syslog (LOG_INFO,"2nd dimm has no magic-key\n");
			pdp->dimm_status = BDBM_ST_DIMMS_UNKNOWN;
			ret = -1;
			goto CLEAN_EXIT;
		}
		uint32_t *tagPtr = (uint32_t *)lmem.virt + 1; /* skip magic-key */
		/* save the dimms' tags */
		dimm_tags[1][0] = tagPtr[0];
		dimm_tags[1][1] = tagPtr[1];
		if ((dimm_tags[1][0] == 0xFFFFFFFF) || \
				(dimm_tags[1][1] == 0xFFFFFFFF)) {
			syslog (LOG_INFO,"Invalid tags found on dimm2: 1. %x, 2. %x\n", \
					dimm_tags[1][0], dimm_tags[1][1]);
			pdp->dimm_status = BDBM_ST_DIMMS_BADTAGS;
			ret = -1;
			goto CLEAN_EXIT;
		} else if ((dimm_tags[0][1] != dimm_tags[1][0]) || \
				(dimm_tags[0][0] != dimm_tags[1][1])) {
			/* if dimm_tags are not matching, print and return error */
			syslog (LOG_INFO,"dimm tags are not as expected\n");
			pdp->dimm_status = BDBM_ST_DIMMS_BADTAGS;
			ret = -1;
			goto CLEAN_EXIT;
		} else if (dimm_tags[0][0] > dimm_tags[1][0]) {
			syslog (LOG_INFO,"Swapping dimm slots not allowed.. \
					Please interchage the curent slots\n");
			pdp->dimm_status = BDBM_ST_DIMMS_SWAPPED;
			ret = -1;
			goto CLEAN_EXIT;
		}
		/* dimm positions are verified */
		memcpy (pdp->dimms.dimm_tags, dimm_tags[0], sizeof (pdp->dimms.dimm_tags));
	}

	if (magic_done == 1) {
		/*go check if we have another dimm*/
		uint32_t *tagPtr = (uint32_t *)lmem.virt + 1; /* skip magic-key */
		if (pdp->dimms.nr_dimms == 2) {
			/* save the dimms' tags */
			dimm_tags[0][0] = tagPtr[0];
			dimm_tags[0][1] = tagPtr[1];
			if ((dimm_tags[0][0] == 0xFFFFFFFF) || \
					(dimm_tags[0][1] == 0xFFFFFFFF)) {
				syslog (LOG_INFO,"Invalid tags found on dimm1: 1. %x, 2. %x\n", \
						dimm_tags[0][0], dimm_tags[0][1]);
				pdp->dimm_status = BDBM_ST_DIMMS_BADTAGS;
				ret = -1;
				goto CLEAN_EXIT;
			}
			spare.row.channel_no = pdp->nand.nr_channels/2;
			magic_done = 2;
			goto NEXT_DIMM;
		} else {
			if (*tagPtr != 0xFFFFFFFF) {
				pdp->dimm_status = BDBM_ST_DIMMS_MISSING;
				syslog (LOG_INFO,"ERROR:Seems one dimm is missing..\n");
				ret = -1;
				goto CLEAN_EXIT;
			}
			goto CHECK_EXIT_ST;
		}
	} else if (magic_done == 2) {
		/* verify the exit status.. if BDBM_EXIT_BAD, need to start rescan
		 * like in the case of new dimm! */
CHECK_EXIT_ST:
		spare_bk.row.page_no = pdp->nand.nr_pages_per_block - 1;
		spare_bk.col = 0;
		df_setup_nand_rd_desc ((void*)&waiter, &spare_bk.row, spare_bk.col, lmem.phy, \
				pdp->nand.page_oob_size, DF_BITS_MGMT_PAGE);
		//setup_nand_rd_desc (&spare, lmem.phy, \
				pdp->nand.page_oob_size, &waiter, &ret);
		sem_wait (&waiter);
		if (*(uint8_t *)lmem.virt == BDBM_ST_EXIT_GOOD) {
			syslog (LOG_INFO,"found good dimm(s)!\n");
			pdp->dimm_status = BDBM_ST_EXIT_GOOD;
			/* allocate 1 node for mgmt_blocks & save magic block addr..
			 * once we know actual number of mgmt blocks, we'll
			 * reallocate mgmt_blocks array */
			bdbm_nblock_st_t *temp;
			pdp->mgmt_blocks = malloc (sizeof(bdbm_nblock_st_t *) * pdp->dimms.nr_dimms);
			for (i=0; i<pdp->dimms.nr_dimms; i++) {
				pdp->mgmt_blocks[i] = malloc (sizeof (bdbm_nblock_st_t));
				temp = pdp->mgmt_blocks[i];
				temp->channel = spare_bk.row.channel_no;
				temp->chip = spare_bk.row.chip_no;
				temp->block = spare_bk.row.block_no;
				spare_bk = spare;
			}
		} else {
			syslog (LOG_INFO,"previous shutdown was unprecedented!\n");
			pdp->dimm_status = BDBM_ST_EXIT_BAD;
			ret = -1;
			goto CLEAN_EXIT;
		}
	} else {
		syslog (LOG_INFO,"fresh ahoy..!\n%d dimm(s) detected\n", \
				pdp->dimms.nr_dimms);
	}

CLEAN_EXIT:
	bdbm_free_pv (&lmem);

	sem_destroy (&waiter);

	return ret;
}

/* list all blocks in b with the good/bad status,
 * start with the addr last mgmt block - 1 from each chip */
void bdbm_scan_bb_completion_cb (void *req_ptr, uint8_t *Buff, uint8_t ret)
{
	bdbm_abm_info_t *bai = _dp.bai;
	bdbm_abm_block_t *b = (bdbm_abm_block_t *)req_ptr;
	if (*(Buff) != 0xFF) {
		b->status = BDBM_ABM_BLK_BAD;
		list_del (&b->list);
		list_add_tail (&b->list, \
				&(bai->list_head_bad[b->channel][b->chip]));
		bai->nr_bad_blks++;
		bai->nr_free_blks--;
	}

}

static int bdbm_rescan_all_blocks (bdbm_dev_private_t *pdp)
{
	int ret = 0;
	int chanCnt = 0, chipCnt = 0, blockCnt = 0;
	bdbm_nandaddr_t blk_addr;
	local_mem_t lmem = {0};
	bdbm_abm_info_t *bai = pdp->bai;
	bdbm_abm_block_t *b = pdp->bai->blocks;
	uint64_t nr_chips = pdp->nand.nr_chips_per_channel;
	uint64_t nr_channels = pdp->nand.nr_channels;
	uint64_t nr_blks = pdp->nand.nr_blocks_per_chip;
	uint64_t blk_nr = 0;
	sem_t waiter;
	bdbm_nblock_st_t *mgmt_block;
	
	sem_init (&waiter, 0, 0);

	bdbm_alloc_pv (&lmem, pdp->nand.page_oob_size);
	blk_addr.row.page_no = 0;
	blk_addr.col = pdp->nand.page_main_size;
	for (blockCnt = 0; blockCnt < nr_blks; blockCnt++) {
		blk_addr.row.block_no = blockCnt;
		for (chipCnt = 0; chipCnt < nr_chips; chipCnt++) {
			blk_addr.row.chip_no = chipCnt;
			for (chanCnt = 0; chanCnt < nr_channels; chanCnt++, b++) {
				blk_addr.row.channel_no = chanCnt;
				blk_nr = (blk_addr.row.channel_no * pdp->nand.nr_blocks_per_channel) + \
						 (blk_addr.row.chip_no * pdp->nand.nr_blocks_per_chip) + \
						 blk_addr.row.block_no;
				df_setup_nand_rd_desc ((void*)&pdp->bai->blocks[blk_nr], &blk_addr.row, blk_addr.col, \
						NULL, pdp->nand.page_oob_size, DF_BITS_MGMT_OOB|DF_BIT_SCAN_BB);
				//setup_nand_rd_desc (&blk_addr, lmem.phy, \
				pdp->nand.page_oob_size, &waiter, &ret);
			}
		}
	}

	while (atomic_read(&act_desc_cnt)) {
		usleep(1);
	}

	pdp->dimm_status = BDBM_ST_RESCANNED;
	pdp->mgmt_blkptr[MGMTB_BMT].nEntries = b - pdp->bai->blocks;

	sem_destroy (&waiter);

	return ret;
}
#define bdbm_load_bmtb_list(pdp) bdbm_load_mgmt_block_list (MGMTB_BMT, pdp)
#define bdbm_load_pmtb_pool(pdp) bdbm_load_mgmt_block_list (MGMTB_PMTB_POOL, pdp)
#define bdbm_load_pmtb_curr(pdp) bdbm_load_mgmt_block_list (MGMTB_PMTB_CURR, pdp)
#define bdbm_load_iobb_pool(pdp) bdbm_load_mgmt_block_list (MGMTB_IOBB_POOL, pdp)

/* load all the blocks in the mgmt_blkptr list specified */
static int bdbm_load_mgmt_block_list (mgmt_list_type_t type, bdbm_dev_private_t *pdp)
{
	int ret = 0;
	int i, j;
	bdbm_nandaddr_t addr;
	local_mem_t lmem = {0};
	bdbm_nblock_st_t *src, *destpbm;
	bdbm_abm_block_t* destabm;
	sem_t waiter;
	uint32_t nEntries = pdp->mgmt_blkptr[type].nEntries;
	uint32_t entriesPerFPage = pdp->nand.page_main_size / \
							   sizeof (bdbm_nblock_st_t);
	uint32_t nFPages = nEntries / entriesPerFPage + \
					   !!(nEntries % entriesPerFPage);

	memcpy (&addr, &pdp->mgmt_blkptr[type].addr, sizeof (addr));

	sem_init (&waiter, 0, 0);

	if (type == MGMTB_BMT) {
		/* typecast and save the bai head ptr, later while storing
		 * we can re-typecast to original and save properly */
		pdp->bmi[type] = (bdbm_nblock_st_t *)pdp->bai->blocks;
		destabm = pdp->bai->blocks;
	} else {
		pdp->bmi[type] = bdbm_malloc (nEntries * sizeof (bdbm_nblock_st_t));
		bdbm_bug_on (pdp->bmi[type] == NULL);
		destpbm = pdp->bmi[type];
	}

	bdbm_alloc_pv (&lmem, pdp->nand.page_main_size);

	for (i = 0; i < nFPages - 1; i++) {
		df_setup_nand_rd_desc ((void*)&waiter, &addr.row, addr.col, lmem.phy, \
				pdp->nand.page_main_size, DF_BITS_MGMT_PAGE);
		//setup_nand_rd_desc (&addr, lmem.phy, \
				pdp->nand.page_main_size, &waiter, &ret);
		sem_wait (&waiter);
		src = (bdbm_nblock_st_t *)lmem.virt;
		if (type != MGMTB_BMT) {
			memcpy (destpbm, src, pdp->nand.page_main_size);
			destpbm += entriesPerFPage;
		} else {
			for (j = 0; j < entriesPerFPage; j++, src++, destabm++) {
				destabm->channel = src->channel;
				destabm->chip = src->chip;
				destabm->block = src->block;
				destabm->erase_count = src->erase_count;
				destabm->nr_invalid_subpages = src->nr_invalid_subpages;
				destabm->status = src->status;
			}
		}
		addr.row.page_no += 1;
		nEntries -= entriesPerFPage;
	}
	df_setup_nand_rd_desc ((void*)&waiter, &addr.row, addr.col, lmem.phy, \
			pdp->nand.page_main_size, DF_BITS_MGMT_PAGE);
	//setup_nand_rd_desc (&addr, lmem.phy, \
			pdp->nand.page_main_size, &waiter, &ret);
	sem_wait (&waiter);
	src = (bdbm_nblock_st_t *)lmem.virt;
	if (type != MGMTB_BMT) {
		memcpy (destpbm, src, nEntries * sizeof (bdbm_nblock_st_t));
	} else {
		for (j = 0; j < nEntries; j++, src++, destabm++) {
			destabm->channel = src->channel;
			destabm->chip = src->chip;
			destabm->block = src->block;
			destabm->erase_count = src->erase_count;
			destabm->nr_invalid_subpages = src->nr_invalid_subpages;
			destabm->status = src->status;
		}
	}

	bdbm_free_pv (&lmem);

	sem_destroy (&waiter);

	return ret;
}

static int bdbm_load_pmt (bdbm_dev_private_t *pdp)
{
	int ret = 0;
	uint32_t nBlks_PmtBCurr = pdp->mgmt_blkptr[MGMTB_PMTB_CURR].nEntries;
	uint32_t pme_cnt = pdp->nand.nr_subpages_per_ssd;
	uint32_t phylen;
	bdbm_nblock_st_t *pmtb_curr = pdp->bmi[MGMTB_PMTB_CURR];
	int i, j, k;
	int entriesPerFPage = pdp->nand.page_main_size / sizeof (bdbm_npme_t );
	bdbm_nandaddr_t addr;
	local_mem_t lmem = {0};
	bdbm_npme_t *src;
	bdbm_pme_t *dest;
	sem_t waiter;
	bdbm_abm_block_t* blks = pdp->bai->blocks;
	uint64_t blk_nr;
	uint64_t page_off;

	sem_init (&waiter, 0, 0);

	bdbm_alloc_pv (&lmem, pdp->nand.page_main_size);

	dest = pdp->pmt;
	addr.col = 0;
	/* starting form pmtb_curr, all the used blocks are considered
	 * to contain pmt upto nBlks_PmtBCurr */
	for (i = 0; i < nBlks_PmtBCurr; i++, pmtb_curr++) {
		addr.row.channel_no = pmtb_curr->channel;
		addr.row.chip_no = pmtb_curr->chip;
		addr.row.block_no = pmtb_curr->block;
		addr.row.page_no = 0;
		for (j = 0; j < pdp->nand.nr_pages_per_block; j++) {
			phylen = pme_cnt * sizeof (bdbm_npme_t);
			if (pme_cnt > entriesPerFPage) {
				phylen = pdp->nand.page_main_size;
			}
			df_setup_nand_rd_desc ((void*)&waiter, &addr.row, addr.col, lmem.phy, \
					phylen, DF_BITS_MGMT_PAGE);
			//setup_nand_rd_desc (&addr, lmem.phy, phylen, &waiter, &ret);
			sem_wait (&waiter);
			src = (bdbm_npme_t *)lmem.virt;
			for (k = 0; k < phylen/sizeof(bdbm_npme_t); k++, dest++, src++) {
				dest->phyaddr.channel_no = src->channel;
				dest->phyaddr.chip_no = src->chip;
				dest->phyaddr.block_no = src->block;
				dest->phyaddr.page_no = src->page;
				dest->sp_off = src->sp_off;
				dest->status = src->status;
				pme_cnt--;
				if (dest->status == PFTL_PAGE_VALID) {
					blk_nr = dest->phyaddr.channel_no * pdp->nand.nr_blocks_per_channel + \
							 dest->phyaddr.chip_no * pdp->nand.nr_blocks_per_chip + \
							 dest->phyaddr.block_no;
					page_off = (dest->phyaddr.page_no * pdp->nand.nr_subpages_per_page) + \
							   (dest->sp_off);
					blks[blk_nr].pst[page_off] = BABM_ABM_SUBPAGE_NOT_INVALID;
				}
			}
			if (phylen < pdp->nand.page_main_size) break;
			addr.row.page_no += 1;
		}
	}

	bdbm_free_pv (&lmem);

	sem_destroy (&waiter);

	return ret;
}

static int bdbm_load_mgmt_ptrs (bdbm_dev_private_t *pdp)
{
	int ret = 0;
	int i = 0;
	uint8_t *lptr;
	bdbm_nandaddr_t addr;
	local_mem_t lmem = {0};
	sem_t waiter;

	sem_init (&waiter, 0, 0);

	/* this means that magic block is already found and 
	 * stored using **mgmt_blocks */
	addr.row.channel_no = (*pdp->mgmt_blocks)->channel;
	addr.row.chip_no = (*pdp->mgmt_blocks)->chip;
	addr.row.block_no = (*pdp->mgmt_blocks)->block;
	addr.row.page_no = MGMTB_MGMT_PTR_PAGE;
	addr.col = 0;

	bdbm_alloc_pv (&lmem, pdp->nand.page_main_size);

	df_setup_nand_rd_desc ((void*)&waiter, &addr.row, addr.col, lmem.phy, \
			pdp->nand.page_main_size, DF_BITS_MGMT_PAGE);
	//setup_nand_rd_desc (&addr, lmem.phy, \
			pdp->nand.page_main_size, &waiter, &ret);
	sem_wait (&waiter);

	for (i = 0, lptr = lmem.virt; i < MGMTB_MAX; i++) {
		memcpy (pdp->mgmt_blkptr + i, lptr, sizeof (bdbm_mgmt_dptr_t));
		lptr += sizeof (bdbm_mgmt_dptr_t);
	}

	bdbm_free_pv (&lmem);

	sem_destroy (&waiter);

	return ret;
}

static void bdbm_update_blks_info_list (bdbm_dev_private_t *pdp)
{
	uint64_t i;
	bdbm_abm_info_t *bai;

	bai = pdp->bai;
	bai->nr_free_blks = 0;
	bai->nr_free_blks_prepared = 0;
	bai->nr_clean_blks = 0;
	bai->nr_dirty_blks = 0;
	bai->nr_mgmt_blks = 0;
	bai->nr_bad_blks = 0;

	for (i = 0; i < bai->np->nr_blocks_per_ssd; i++) {
		bdbm_abm_block_t *b = &bai->blocks[i];
		list_del (&b->list);
		switch (b->status) {
			case BDBM_ABM_BLK_FREE:
				list_add_tail (&b->list, &(bai->list_head_free[b->channel][b->chip]));
				bai->nr_free_blks++;
				break;
			case BDBM_ABM_BLK_FREE_PREPARE:
				list_add_tail (&b->list, &(bai->list_head_free[b->channel][b->chip]));
				bai->nr_free_blks_prepared++;
				break;
			case BDBM_ABM_BLK_CLEAN:
				list_add_tail (&b->list, &(bai->list_head_clean[b->channel][b->chip]));
				bai->nr_clean_blks++;
				break;
			case BDBM_ABM_BLK_DIRTY:
				list_add_tail (&b->list, &(bai->list_head_dirty[b->channel][b->chip]));
				bai->nr_dirty_blks++;
				bdbm_memset (b->pst, BDBM_ABM_SUBPAGE_INVALID, sizeof (babm_abm_subpage_t) \
						* bai->np->nr_pages_per_block);
				break;
			case BDBM_ABM_BLK_MGMT:
				list_add_tail (&b->list, &(bai->list_head_mgmt[b->channel][b->chip]));
				bai->nr_mgmt_blks++;
				break;

			case BDBM_ABM_BLK_BAD:
				list_add_tail (&b->list, &(bai->list_head_bad[b->channel][b->chip]));
				bai->nr_bad_blks++;
				break;
			default:
				bdbm_error ("invalid block type: blk-id = %lu, blk-status = %u", i, b->status);
				break;
		}
	}

	syslog (LOG_INFO,"abm-load: free:%lu, free(prepare):%lu, clean:%lu, dirty:%lu, mgmt:%lu bad:%lu",
			bai->nr_free_blks, 
			bai->nr_free_blks_prepared, 
			bai->nr_clean_blks,
			bai->nr_dirty_blks,
			bai->nr_mgmt_blks,
			bai->nr_bad_blks);

}

static void bdbm_get_mgmt_blk_info_list(bdbm_dev_private_t *pdp)
{
	uint8_t chan, chip;
	uint16_t blk; 
	uint32_t punit, idx, punit_mgmt_blks;
	uint64_t blk_nr;
	punit_mgmt_blks = pdp->nr_mgmt_blocks_per_ssd/pdp->nand.nr_chips_per_ssd;
	for (punit = 0; punit < pdp->nand.nr_chips_per_ssd; punit++) {
		for (idx = 0; idx < punit_mgmt_blks; idx++) {
			bdbm_abm_block_t* b = NULL;
			chan = pdp->mgmt_blocks[punit][idx].channel;
			chip = pdp->mgmt_blocks[punit][idx].chip;
			blk  = pdp->mgmt_blocks[punit][idx].block;

			blk_nr = (chan * pdp->nand.nr_blocks_per_channel) + \
					 (chip * pdp->nand.nr_blocks_per_chip) + blk;
			
			b = &pdp->bai->blocks[blk_nr]; 
			list_del (&b->list);
			b->status = BDBM_ABM_BLK_MGMT;
			list_add_tail (&b->list, &(pdp->bai->list_head_mgmt[b->channel][b->chip]));
			pdp->bai->nr_mgmt_blks++;
			pdp->bai->nr_free_blks--;
		}
	}
}

int bdbm_ftl_load_from_flash (bdbm_dev_private_t *pdp)
{
	int ret = 0;

	if (pdp->dimm_status == BDBM_ST_EXIT_GOOD) {
		JUMP_ON_ERR ((bdbm_load_mgmt_ptrs (pdp)), ret, LONGER_ROUTE, \
				"bdbm_load_mgmt_ptrs failed");
#ifdef MGMT_DATA_ON_NAND
		JUMP_ON_ERR (bdbm_load_bmtb_list (pdp), ret, LONGER_ROUTE, \
				"bdbm_load_bmtb_list failed");
		bdbm_update_blks_info_list (pdp);
#endif
		JUMP_ON_ERR (bdbm_load_pmtb_pool (pdp), ret, LONGER_ROUTE, \
				"bdbm_load_pmtb_pool failed");
		JUMP_ON_ERR (bdbm_load_pmtb_curr (pdp), ret, LONGER_ROUTE, \
				"bdbm_load_pmtb_curr failed");

 		JUMP_ON_ERR (bdbm_load_iobb_pool(pdp), ret, LONGER_ROUTE, \
				"bdbm_load_iobb_pool failed");
		/* erase and rebadge the dimm(s) */
		JUMP_ON_ERR (bdbm_badge_dimms (pdp), ret, LONGER_ROUTE, \
				"bdbm_badge_dimms failed");

#ifdef MGMT_DATA_ON_NAND
		JUMP_ON_ERR (bdbm_load_pmt (pdp), ret, LONGER_ROUTE, \
				"bdbm_load_pmt failed");
#endif
		pdp->dimm_status = BDBM_ST_LOAD_SUCCESS;
		return ret;
	}

LONGER_ROUTE:
	ret = bdbm_rescan_all_blocks (pdp);
	bdbm_get_mgmt_blk_info_list(pdp);
	return ret;
}

/****************************BDBM_STORE**********************************/
int bdbm_badge_dimms (bdbm_dev_private_t *pdp)
{
	int ret = 0;
	uint32_t *tagPtr;
	bdbm_nandaddr_t magic;
	local_mem_t lmem = {0};
	sem_t waiter;

	syslog (LOG_INFO,"badging %s dimm(s)\n", \
			(pdp->dimm_status == BDBM_ST_EXIT_GOOD)?"Old":"as New");

	sem_init (&waiter, 0, 0);

	bdbm_alloc_pv (&lmem, pdp->nand.page_oob_size);

	/* what if magic block goes bad? -TODO */
	magic.row.channel_no = pdp->mgmt_blocks[0][0].channel;
	magic.row.chip_no = pdp->mgmt_blocks[0][0].chip;
	magic.row.block_no = pdp->mgmt_blocks[0][0].block;
	magic.row.page_no = 0;
	magic.col = pdp->nand.page_main_size;

	*(uint32_t *)lmem.virt = MAGIC_KEY;

	tagPtr = (uint32_t *)lmem.virt + 1;

	if (pdp->dimms.nr_dimms == 2) {
		/* tags come into picture now */
		tagPtr[0] = pdp->dimms.dimm_tags[0];
		tagPtr[1] = pdp->dimms.dimm_tags[1];
	}
	df_setup_nand_erase_desc ((void*)&waiter, &magic.row, magic.col, \
			NULL, 0, DF_BIT_MGMT_IO);
	//setup_nand_erase_desc (&magic, NULL, 0, &waiter, &ret);
	sem_wait (&waiter);
	bdbm_bug_on (ret != 0);
	/*Send dummy main page write command in order to write on spare area alone*/
	df_setup_nand_wr_desc (NULL, &magic.row, 0, \
			NULL, 0, DF_BITS_MGMT_PAGE);

	df_setup_nand_wr_desc ((void*)&waiter, &magic.row, magic.col, \
			lmem.phy, pdp->nand.page_oob_size, DF_BITS_MGMT_OOB);
	//setup_nand_wr_desc (&magic, lmem.phy, \
			pdp->nand.page_oob_size, &waiter, &ret);
	sem_wait (&waiter);
	bdbm_bug_on (ret != 0);

	if (pdp->dimms.nr_dimms == 2) {
		uint32_t magicblk2_idx = pdp->nand.nr_chips_per_ssd/2;

		tagPtr[0] = pdp->dimms.dimm_tags[1];
		tagPtr[1] = pdp->dimms.dimm_tags[0];

		if (pdp->dimm_status == BDBM_ST_EXIT_GOOD) {
			magic.row.channel_no = pdp->mgmt_blocks[1][0].channel;
			magic.row.chip_no = pdp->mgmt_blocks[1][0].chip;
			magic.row.block_no = pdp->mgmt_blocks[1][0].block;
		} else {
			magic.row.channel_no = pdp->mgmt_blocks[magicblk2_idx][0].channel;
			magic.row.chip_no = pdp->mgmt_blocks[magicblk2_idx][0].chip;
			magic.row.block_no = pdp->mgmt_blocks[magicblk2_idx][0].block;
		}
		df_setup_nand_erase_desc ((void*)&waiter, &magic.row, magic.col, \
				NULL, 0, DF_BIT_MGMT_IO);
		//setup_nand_erase_desc (&magic, NULL, 0, &waiter, &ret);
		sem_wait (&waiter);
		bdbm_bug_on (ret != 0);

		/*Send dummy main page write command in order to write on spare area alone*/
		df_setup_nand_wr_desc (NULL, &magic.row, 0, \
				NULL, 0, DF_BITS_MGMT_PAGE);

		df_setup_nand_wr_desc ((void*)&waiter, &magic.row, magic.col, \
				lmem.phy, pdp->nand.page_oob_size, DF_BITS_MGMT_OOB);
		//setup_nand_wr_desc (&magic, lmem.phy, \
				pdp->nand.page_oob_size, &waiter, &ret);
		sem_wait (&waiter);
		bdbm_bug_on (ret != 0);
		magic.row.channel_no = 0;
	}

	bdbm_free_pv (&lmem);

	sem_destroy (&waiter);

	return ret;
}

int bdbm_store_pmt (bdbm_dev_private_t *pdp)
{
	int ret = 0;
	uint32_t nBlks_PmtBCurr = pdp->mgmt_blkptr[MGMTB_PMTB_CURR].nEntries;
	uint32_t nBlks_PmtBPool = pdp->mgmt_blkptr[MGMTB_PMTB_POOL].nEntries;
	bdbm_nblock_st_t *pmtb_curr = pdp->bmi[MGMTB_PMTB_CURR];
	bdbm_nblock_st_t *pmtb_pool = pdp->bmi[MGMTB_PMTB_POOL];
	bdbm_nblock_st_t *curr_base;
	int i = 0, j = 0, k = 0;
	uint32_t entriesPerFPage = pdp->nand.page_main_size / sizeof (bdbm_npme_t);
	uint32_t phylen;
	uint32_t pme_cnt = pdp->nand.nr_subpages_per_ssd;
	bdbm_nandaddr_t addr;
	local_mem_t lmem = {0};
	bdbm_pme_t *src;
	bdbm_npme_t *dest;
	sem_t waiter;

	sem_init (&waiter, 0, 0);

	bdbm_alloc_pv (&lmem, pdp->nand.page_main_size);

	if (pdp->dimm_status != BDBM_ST_RESCANNED) {
		/* find the idx from where curr is present in pool */
		for (i = 0; i < nBlks_PmtBPool; i++) {
			if (!memcmp (pmtb_curr, &pmtb_pool[i], \
						sizeof (bdbm_nblock_st_t))) {
				break;
			}
		}
		/* mark the status of the blocks already used as dirty */
		for (j = 0; j < nBlks_PmtBCurr; j++) {
			pmtb_pool[i].status = BDBM_ABM_BLK_DIRTY;
			CIRC_INCR (i, nBlks_PmtBPool);
		}
	}
	curr_base = &pmtb_pool[i];
	pmtb_curr = curr_base;

	src = pdp->pmt;
	addr.col = 0;
	/* starting form pmtb_pool[i], all the used blocks are going 
	 * to contain pmt till nBlks_PmtBCurr */
	for (i = 0; i < nBlks_PmtBCurr; i++) {
ERASURE:
		addr.row.channel_no = pmtb_curr->channel;
		addr.row.chip_no = pmtb_curr->chip;
		addr.row.block_no = pmtb_curr->block;
		addr.row.page_no = 0;
		df_setup_nand_erase_desc ((void*)&waiter, &addr.row, \
				addr.col, NULL, 0, DF_BIT_MGMT_IO);
		//setup_nand_erase_desc (&addr, lmem.phy, \
				pdp->nand.page_main_size, &waiter, &ret);
		sem_wait (&waiter);
		if (ret) {
			int ntemp = pdp->mgmt_blkptr[MGMTB_PMTB_POOL].nEntries - \
						(pmtb_curr - pmtb_pool);
			memcpy (pmtb_curr, pmtb_curr + 1, \
					ntemp * sizeof (bdbm_nblock_st_t));
			memset (pmtb_curr + ntemp, 0, sizeof (bdbm_nblock_st_t));
			pdp->mgmt_blkptr[MGMTB_PMTB_POOL].nEntries--;
			CIRC_INCR_PTR (pmtb_curr, pmtb_pool, \
					pdp->mgmt_blkptr[MGMTB_PMTB_POOL].nEntries);
			syslog (LOG_INFO,"lost 1 good mgmt block from pmtb_free_pool!\n");
			goto ERASURE;
		}
		for (j = 0; j < pdp->nand.nr_pages_per_block; j++) {
			phylen = pme_cnt * sizeof (bdbm_npme_t);
			if (pme_cnt > entriesPerFPage) {
				phylen = pdp->nand.page_main_size;
			}
			dest = (bdbm_npme_t *)lmem.virt;
			for (k = 0; k < phylen/sizeof(bdbm_npme_t); k++, dest++, src++) {
				dest->channel = src->phyaddr.channel_no;
				dest->chip = src->phyaddr.chip_no;
				dest->block = src->phyaddr.block_no;
				dest->page = src->phyaddr.page_no;
				dest->sp_off = src->sp_off;
				dest->status = src->status;
				pme_cnt--;
			}
			df_setup_nand_wr_desc ((void*)&waiter, &addr.row, addr.col, lmem.phy, \
					pdp->nand.page_main_size, DF_BITS_MGMT_PAGE);
			//setup_nand_wr_desc (&addr, lmem.phy, \
					pdp->nand.page_main_size, &waiter, &ret);
			sem_wait (&waiter);
			addr.row.page_no += 1;
		}
		CIRC_INCR_PTR (pmtb_curr, pmtb_pool, nBlks_PmtBPool);
	}

	/* update the pmtb_curr */
	pmtb_curr = pdp->bmi[MGMTB_PMTB_CURR];
	/*
	for (j = 0, i = 0; j < nBlks_PmtBCurr; j++) {
		memcpy (pmtb_curr + i, curr_base + i, sizeof (bdbm_nblock_st_t));
		CIRC_INCR (i, nBlks_PmtBPool);
	}
	*/
	for (j = 0, i = 0; j < nBlks_PmtBCurr; j++) {
		memcpy (pmtb_curr + i, curr_base, sizeof (bdbm_nblock_st_t));
		CIRC_INCR (i, nBlks_PmtBPool);
		CIRC_INCR_PTR (curr_base, pmtb_pool, nBlks_PmtBPool);
	}
	
	bdbm_free_pv (&lmem);

	sem_destroy (&waiter);

	return ret;
}

#define bdbm_store_bmtb_list(pdp) bdbm_store_mgmt_block_list (MGMTB_BMT, pdp)
#define bdbm_store_pmtb_pool(pdp) bdbm_store_mgmt_block_list (MGMTB_PMTB_POOL, pdp)
#define bdbm_store_pmtb_curr(pdp) bdbm_store_mgmt_block_list (MGMTB_PMTB_CURR, pdp)
#define bdbm_store_iobb_pool(pdp) bdbm_store_mgmt_block_list (MGMTB_IOBB_POOL, pdp)

static int bdbm_store_mgmt_block_list (mgmt_list_type_t type, bdbm_dev_private_t *pdp)
{
	int ret = 0;
	int i, j;
	bdbm_nandaddr_t addr;
	local_mem_t lmem = {0};
	bdbm_nblock_st_t *dest, *srcpbm;
	bdbm_abm_block_t* srcabm;
	sem_t waiter;
	uint32_t nEntries = pdp->mgmt_blkptr[type].nEntries;
	uint32_t entriesPerFPage = pdp->nand.page_main_size / \
							   sizeof (bdbm_nblock_st_t);
	uint32_t nFPages = nEntries / entriesPerFPage + \
					   !!(nEntries % entriesPerFPage);

	memcpy (&addr.row, &pdp->mgmt_blkptr[type].addr, sizeof (addr));

	sem_init (&waiter, 0, 0);

	if (type == MGMTB_BMT) {
		/* re-typecast to original and use */
		srcabm = (bdbm_abm_block_t *)pdp->bmi[type];
	} else {
		bdbm_bug_on (pdp->bmi[type] == NULL);
		srcpbm = pdp->bmi[type];
	}

	bdbm_alloc_pv (&lmem, pdp->nand.page_main_size);
	for (i = 0; i < nFPages - 1; i++) {
		dest = (bdbm_nblock_st_t *)lmem.virt;
		if (type != MGMTB_BMT) {
			memcpy (dest, srcpbm, pdp->nand.page_main_size);
			srcpbm += entriesPerFPage;
		} else {
			for (j = 0; j < entriesPerFPage; j++, dest++, srcabm++) {
				dest->channel = srcabm->channel;
				dest->chip = srcabm->chip;
				dest->block = srcabm->block;
				dest->erase_count = srcabm->erase_count;
				dest->nr_invalid_subpages = srcabm->nr_invalid_subpages;
				dest->status = srcabm->status;

			}
		}
		df_setup_nand_wr_desc ((void*)&waiter, &addr.row, addr.col, lmem.phy, \
				pdp->nand.page_main_size, DF_BITS_MGMT_PAGE);
		//setup_nand_wr_desc (&addr, lmem.phy, \
				pdp->nand.page_main_size, &waiter, &ret);
		sem_wait (&waiter);
		addr.row.page_no += 1;
		nEntries -= entriesPerFPage;
	}
	dest = (bdbm_nblock_st_t *)lmem.virt;

	if (type != MGMTB_BMT) {
		memcpy (dest, srcpbm, nEntries * sizeof (bdbm_nblock_st_t));
	} else {
		for (j = 0; j < nEntries; j++, dest++, srcabm++) {
			dest->channel = srcabm->channel;
			dest->chip = srcabm->chip;
			dest->block = srcabm->block;
			dest->erase_count = srcabm->erase_count;
			dest->nr_invalid_subpages = srcabm->nr_invalid_subpages;
			dest->status = srcabm->status;

		}
	}
	df_setup_nand_wr_desc ((void*)&waiter, &addr.row, addr.col, lmem.phy, \
			pdp->nand.page_main_size, DF_BITS_MGMT_PAGE);
	//setup_nand_wr_desc (&addr, lmem.phy, \
			pdp->nand.page_main_size, &waiter, &ret);
	sem_wait (&waiter);

	bdbm_free_pv (&lmem);

	sem_destroy (&waiter);

	return ret;
}

static int bdbm_store_mgmt_ptrs (bdbm_dev_private_t *pdp)
{
	int ret = 0;
	int i;
	bdbm_mgmt_dptr_t *lptr;
	bdbm_nandaddr_t addr;
	local_mem_t lmem = {0};
	sem_t waiter;

	sem_init (&waiter, 0, 0);

	addr.row.channel_no = pdp->mgmt_blocks[0][0].channel;
	addr.row.chip_no = pdp->mgmt_blocks[0][0].chip;
	addr.row.block_no = pdp->mgmt_blocks[0][0].block;
	addr.row.page_no = MGMTB_MGMT_PTR_PAGE;
	addr.col = 0;

	bdbm_alloc_pv (&lmem, pdp->nand.page_main_size);

	lptr = (bdbm_mgmt_dptr_t *)lmem.virt;

	for (i = 0; i < MGMTB_MAX; i++, lptr++) {
		memcpy (lptr, &pdp->mgmt_blkptr[i], sizeof (bdbm_mgmt_dptr_t));
	}

	df_setup_nand_wr_desc ((void*)&waiter, &addr.row, addr.col, lmem.phy, \
			pdp->nand.page_main_size, DF_BITS_MGMT_PAGE);
	//setup_nand_wr_desc (&addr, lmem.phy, \
			pdp->nand.page_main_size, &waiter, &ret);
	sem_wait (&waiter);

	sem_destroy (&waiter);

	bdbm_free_pv (&lmem);

	return ret;
}

static int bdbm_store_exit_status (bdbm_dev_private_t *pdp)
{
	int ret = 0;
	bdbm_nandaddr_t addr;
	local_mem_t lmem = {0};
	sem_t waiter;

	sem_init (&waiter, 0, 0);

	addr.row.channel_no = pdp->mgmt_blocks[0][0].channel;
	addr.row.chip_no = pdp->mgmt_blocks[0][0].chip;
	addr.row.block_no = pdp->mgmt_blocks[0][0].block;
	addr.row.page_no = pdp->nand.nr_pages_per_block - 1;
	addr.col = 0;

	bdbm_alloc_pv (&lmem, pdp->nand.page_oob_size);

	*lmem.virt = BDBM_ST_EXIT_GOOD;

	df_setup_nand_wr_desc ((void*)&waiter, &addr.row, addr.col, lmem.phy, \
			pdp->nand.page_oob_size, DF_BITS_MGMT_PAGE);
	//setup_nand_wr_desc (&addr, lmem.phy, \
			pdp->nand.page_oob_size, &waiter, &ret);
	sem_wait (&waiter);

	sem_destroy (&waiter);

	bdbm_free_pv (&lmem);

	return ret;
}

int bdbm_ftl_store_to_flash (bdbm_dev_private_t *pdp)
{
	int ret = 1;

	if ((pdp->dimm_status == BDBM_ST_DIMMS_NEW) || \
			(pdp->dimm_status == BDBM_ST_EXIT_GOOD) || \
			(pdp->dimm_status == BDBM_ST_RESCANNED) || \
			(pdp->dimm_status == BDBM_ST_LOAD_SUCCESS)) {
#ifdef MGMT_DATA_ON_NAND
		JUMP_ON_ERR (bdbm_store_pmt (pdp), ret, RET, \
				"bdbm_store_pmt failed");
#endif	
		JUMP_ON_ERR (bdbm_store_mgmt_ptrs (pdp), ret, RET, \
				"bdbm_store_mgmt_ptrs failed");
		
		JUMP_ON_ERR (bdbm_store_pmtb_curr (pdp), ret, RET, \
				"bdbm_store_pmtb_curr failed");
		JUMP_ON_ERR (bdbm_store_pmtb_pool (pdp), ret, RET, \
				"bdbm_store_pmtb_pool failed");
#ifdef MGMT_DATA_ON_NAND
		JUMP_ON_ERR (bdbm_store_bmtb_list (pdp), ret, RET, \
				"bdbm_store_bmtb_list failed");
#endif	

		JUMP_ON_ERR (bdbm_store_iobb_pool(pdp), ret, RET, \
				"bdbm_store_iobb_pool failed");

		JUMP_ON_ERR (bdbm_store_exit_status (pdp), ret, RET, \
				"bdbm_store_exit_status failed");
	}

RET:
	return (!ret ? BDBM_ST_STORE_SUCCESS : BDBM_ST_STORE_FAILED);
}
