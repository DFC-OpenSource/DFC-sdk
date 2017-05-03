/**
 * @file      : cmd_defn.c
 * @brief     : This file contains the definition of each command handler
 * @author    : Sakthi Kannan R (sakthikannan.r@vvdntech.com)
 * @copyright : (c) 2012-2014 , VVDN Technologies Pvt. Ltd.
 *              Permission is hereby granted to everyone in VVDN Technologies
 *              to use the Software without restriction, including without
 *              limitation the rights to use, copy, modify, merge, publish,
 *              distribute, distribute with modifications.
 */

#include "cli_parser.h"
#include "cmd_defn.h"
#include "nvme.h"
#include "telnetd.h"
#include "bdbm_ftl/bdbm_drv.h"
#include "bdbm_ftl/params.h"
#include "bdbm_ftl/ftl/abm.h"
#include "bdbm_ftl/list.h"

extern NvmeCtrl g_NvmeCtrl;
extern struct bdbm_drv_info* _bdi;

int wearing_info (U32 argc, char **argv, contextInfo_t *c)
{
	uint64_t loop, page_no, channel, chip;
	FILE *fp = NULL;
	struct nand_params* np = &_bdi->ptr_bdbm_params->nand;
	struct bdbm_abm_info* bai;
	struct bdbm_abm_block_t* blk = NULL;
	struct bdbm_abm_block_t** ac_bab = NULL;
	struct list_head* pos = NULL;
	bai = bdbm_get_bai(_bdi);
	ac_bab = bdbm_get_ac_bab(_bdi);


	if ((fp = fopen(argv[1],"a+")) == NULL) {
		cprintf(c,"\nError in opening a file");
		return 0;
	}
	
	loop = 0;
	fprintf(fp,"\n>>>>>BLOCKS COUNT<<<<<\nTotal Blocks : %lu",bai->nr_total_blks);
	fprintf(fp,"\nFree Blocks : %lu",bai->nr_free_blks);
	for (channel = 0; channel < np->nr_channels; channel++) {
		for (chip = 0;chip < np->nr_chips_per_channel; chip++) {
			list_for_each (pos, &(bai->list_head_free[channel][chip])) {
				blk = list_entry (pos, struct bdbm_abm_block_t, list);
				fprintf(fp,"\nChannel : %lu  Chip : %lu  Blk : %lu",blk->channel_no,blk->chip_no,blk->block_no);
				loop++;
			}
		}
	}
	fprintf(fp,"\nFree Blocks count : %lu",loop);
	
	loop = 0;
	fprintf(fp,"\n\nclean Blocks : %lu",bai->nr_clean_blks);
	for (channel = 0; channel < np->nr_channels; channel++) {
		for (chip = 0;chip < np->nr_chips_per_channel; chip++) {
			list_for_each (pos, &(bai->list_head_clean[channel][chip])) {
				blk = list_entry (pos, struct bdbm_abm_block_t, list);
				fprintf(fp,"\nChannel : %lu  Chip : %lu  Blk : %lu",blk->channel_no,blk->chip_no,blk->block_no);
				loop++;
			}
		}
	}
	fprintf(fp,"\nClean Blocks count : %lu",loop);

	loop = 0;
	fprintf(fp,"\n\ndirty Blocks : %lu",bai->nr_dirty_blks);
	for (channel = 0; channel < np->nr_channels; channel++) {
		for (chip = 0;chip < np->nr_chips_per_channel; chip++) {
			list_for_each (pos, &(bai->list_head_dirty[channel][chip])) {
				blk = list_entry (pos, struct bdbm_abm_block_t, list);
				fprintf(fp,"\nChannel : %lu  Chip : %lu  Blk : %lu",blk->channel_no,blk->chip_no,blk->block_no);
				loop++;
			}
		}
	}
	fprintf(fp,"\nDirty Blocks count : %lu",loop);

	loop = 0;
	fprintf(fp,"\n\nManagement Blocks : %lu",bai->nr_mgmt_blks);
	for (channel = 0; channel < np->nr_channels; channel++) {
		for (chip = 0;chip < np->nr_chips_per_channel; chip++) {
			list_for_each (pos, &(bai->list_head_mgmt[channel][chip])) {
				blk = list_entry (pos, struct bdbm_abm_block_t, list);
				fprintf(fp,"\nChannel : %lu  Chip : %lu  Blk : %lu",blk->channel_no,blk->chip_no,blk->block_no);
				loop++;
			}
		}
	}
	fprintf(fp,"\nManagement Blocks count : %lu",loop);

	loop = 0;
	fprintf(fp,"\n\nBad Blocks : %lu",bai->nr_bad_blks);
	for (channel = 0; channel < np->nr_channels; channel++) {
		for (chip = 0;chip < np->nr_chips_per_channel; chip++) {
			list_for_each (pos, &(bai->list_head_bad[channel][chip])) {
				blk = list_entry (pos, struct bdbm_abm_block_t, list);
				fprintf(fp,"\nChannel : %lu  Chip : %lu  Blk : %lu",blk->channel_no,blk->chip_no,blk->block_no);
				loop++;
			}
		}
	}
	fprintf(fp,"\nBad Blocks count : %lu",loop);

	fprintf(fp,"\n\nActive blocks list");
	for (loop = 0; loop < np->nr_channels * np->nr_chips_per_channel; loop++) {
		fprintf(fp,"\nChannel : %lu  Chip : %lu  Blk : %lu  Status : %d  Erase : %u  Invalid_pge : %d", \
				ac_bab[loop]->channel_no,ac_bab[loop]->chip_no,ac_bab[loop]->block_no, \
				ac_bab[loop]->status,ac_bab[loop]->erase_count,ac_bab[loop]->nr_invalid_subpages);
	}

	fprintf(fp,"\n\n\n>>>>>BLOCK INFO<<<<<");
	for (loop = 0; loop < np->nr_blocks_per_ssd; loop++) {
		fprintf(fp,"\nChannel : %lu  Chip : %lu  Blk : %lu",bai->blocks[loop].channel_no, \
				bai->blocks[loop].chip_no,bai->blocks[loop].block_no);
		fprintf(fp,"\n1. Erase count : %u\n2. Number of invalid pages : %d", \
				bai->blocks[loop].erase_count,bai->blocks[loop].nr_invalid_subpages);
		fprintf(fp,"\n3. Block status : %d",bai->blocks[loop].status);
		fprintf(fp,"\n4. Page status :\n{\n");
		for (page_no = 0; page_no < np->nr_pages_per_block; page_no++) {
			fprintf(fp,"%d ",bai->blocks[loop].pst[page_no]);
		}
		fprintf(fp,"\n}\n\n\n");
		fprintf(fp,"\n\n");
	}
	fclose(fp);
	return 0;
}

int switch_mode_su_cmd (U32 argc, char **argv, contextInfo_t *c)
{
	if (c->isAdmin == FALSE) {
		c->isAdmin = TRUE;
		cprintf (c, "Switching to super-user mode\r\n");
	}
	return 0;
}

int switch_mode_user_cmd (U32 argc, char **argv, contextInfo_t *c)
{
	if (c->isAdmin == TRUE) {
		c->isAdmin = FALSE;
		c->isDebug = FALSE;
		cprintf (c, "Switching to user mode\r\n");
	}
	return 0;
}

int get_info (U32 argc, char **argv, contextInfo_t *c)
{   
	return 0;
}

int get_info_all (U32 argc, char **argv, contextInfo_t *c)
{ 
	cprintf(c,"\n\t\tGET INFO\n");
	get_general_info (argc,argv,c);
	get_register_info (argc,argv,c);
	get_command_info (argc,argv,c);
	get_feature_info (argc,argv,c);

	return 0;
}

int get_general_info (U32 argc, char **argv, contextInfo_t *c)
{
	NvmeCtrl *n = &g_NvmeCtrl;

	cprintf(c,"\n\t\tGENERAL INFO\nNumber of namespaces(NN): %u \
			\nNumber of Queues: %u	\
			\nMaximum number of Entries: %u \
			\nDoorbell stride (DSTRD): %u \
			\nAsynchronous Event Request Limit (AERL): %u \
			\nAbort Command Limit (ACL): %u \
			\nError Log Page Entries (ELPE): %u \
			\nMaximum Data Transfer Size (MDTS): %u \
			\nMemory Page size (MPS): %u \
			\nI/O Completion Queue Entry Size (IOCQES): %u \
			\nI/O submission Queue Entry Size (IOSQES): %u \
			\nPCI Vendor ID (VID):0x%x \
			\nPCI Device ID(DID): 0x%x \
			\nTemperature: 0x%x \
			\nMaximum PRP entries: %u \
			\nOptional Admin Command Support: %u \
			\nOptional NVM Command Support: %u \
			\nNamespace Size: %llu \
			\nNumber of errors: %u \
			\nNumber of Outstanding Asynchronous requests: %u \n", \
			n->num_namespaces,n->num_queues,n->max_q_ents, \
			n->db_stride,n->aerl,n->acl, \
			n->elpe,n->mdts,n->page_size,n->cqe_size,n->sqe_size, \
			n->vid,n->did,n->temperature,n->max_prp_ents,n->oacs, \
			n->oncs,n->ns_size,n->num_errors,n->outstanding_aers);

	return 0;
}

int get_register_info (U32 argc, char **argv, contextInfo_t *c)
{
	NvmeCtrl *n = &g_NvmeCtrl;

	cprintf(c,"\n\t\tREGISTER INFO");
	cprintf(c,"\nController Capabilities (CAP): 0x%lx \
			\nVersion (VS): 0x%x \
			\nInterrupt Mask Set (INTMS): 0x%x \
			\nInterrupt Mask Clear (INTMC): 0x%x \
			\nController Configuration (CC): 0x%x \
			\nController Status (CSTS): 0x%x \
			\nNVM Subsystem Reset (NSSR): 0x%x \
			\nAdmin Queue Attributes (AQA): 0x%x \
			\nAdmin Submission Queue Base Address (ASQ): 0x%lx \
			\nAdmin Completion Queue Base Address (ACQ): 0x%lx \
			\nController Memory Buffer Location (CMBLOC): 0x%x \
			\nController Memory Buffer Size (CMBSZ): 0x%x \
			\n", n->nvme_regs.vBar.cap,n->nvme_regs.vBar.vs,n->nvme_regs.vBar.intms, \
			n->nvme_regs.vBar.intmc,n->nvme_regs.vBar.cc,n->nvme_regs.vBar.csts, \
			n->nvme_regs.vBar.nssrc,n->nvme_regs.vBar.aqa,n->nvme_regs.vBar.asq, \
			n->nvme_regs.vBar.acq,n->nvme_regs.vBar.cmbloc,n->nvme_regs.vBar.cmbsz);

	return 0;
}

int get_command_info (U32 argc, char **argv, contextInfo_t *c)
{
	NvmeCtrl *n = &g_NvmeCtrl;
	int qid = 0;
	NvmeSQ *sq;

	cprintf(c,"\n\t\tCOMMAND INFO\n");
	cprintf(c,"\nTotal number of Admin Commands Submitted: %u\n \
			\nTotal number of IO Commands Submitted: %u\n", \
			n->stat.tot_num_AdminCmd,n->stat.tot_num_IOCmd);

	cprintf(c,"\nTotal number of Read commands posted: %u\n",n->stat.nr_read_ops[0]);
	cprintf(c,"Total number of write commands posted: %u\n",n->stat.nr_write_ops[0]);
	sq = n->sq[qid];
    if(sq == NULL) {
        printf("Error: sq is null\n");
        return -1;
    }
	for (qid = 1; qid <= n->stat.num_active_queues; qid++) {
		sq = n->sq[qid];
		cprintf(c,"\nNumber of commands posted in Q%d: %u\n",qid,sq->posted);
		cprintf(c,"Number of commands completed in Q%d: %lu\n",qid,sq->completed);
		cprintf(c,"Number of commands to be processed in Q%d: %u\n",qid,(sq->posted - sq->completed));
	}
	return 0;
}

int get_feature_info (U32 argc, char **argv, contextInfo_t *c)
{
	NvmeCtrl *n = &g_NvmeCtrl;
	int i;

	cprintf(c,"\n\t\tFEATURE INFO");
	cprintf(c,"\nArbitration: %u \
			\nPower Management: %u \
			\nTemperature Threshold: %u \
			\nError Recovery: %u \
			\nNumber of Queues: %u \
			\nInterrupt Coalescing: %u \
			\nWrite Atomicity Normal: %u \
			\nAsynchronous Event Configuration: %u \
			\n",n->features.arbitration,n->features.power_mgmt, \
			n->features.temp_thresh,n->features.err_rec,n->features.num_queues, \
			n->features.int_coalescing,n->features.write_atomicity,n->features.async_config);

	for (i = 1; i <= n->num_queues ; i++) {
		cprintf(c,"Interrupt Vector Configuration for QID%d : %u\n",i,n->features.int_vector_config[i]);
	}
	cprintf(c,"Volatile Write Cache: %u\nSoftware Progress Marker: %u\n",n->features.volatile_wc,n->features.sw_prog_marker);

	return 0;
}

int clear_screen (U32 argc, char **argv, contextInfo_t *c)
{
#ifdef __linux__
	return (system("clear"));
#else
#ifdef uC
	cprintf (c, "\033[H\033[J"); /* Terminal ASCII seq for clear screen */
	return 0;
#endif
#endif
}

#define DEBUG_MODE_PASS "pd*nz4hjHzMy7&g"
#define MAX_PASS_CHAR sizeof(DEBUG_MODE_PASS)
int enable_debug_cmd (U32 argc, char **argv, contextInfo_t *c)
{
	char pass[MAX_PASS_CHAR+1];
	U8 len = 0;
	if (c->isDebug == FALSE) {
		cprintf(c, "Enter password: ");
#if !(defined CHARACTER_MODE) && (defined __linux__)
		fflush (stdout);
#endif /* #if !(defined CHARACTER_MODE) && (defined __linux__) */
		len = cgetstr (c, pass, HIDDEN_CHAR, MAX_PASS_CHAR);
		if ((len == MAX_PASS_CHAR) &&
				(strncmp(pass, DEBUG_MODE_PASS, MAX_PASS_CHAR) == 0)) {
			cprintf (c, "\r\nEnabling debug-mode\r\n");
			c->isDebug = TRUE;
		} else {
			cprintf (c, "\r\nInvalid Password!!!\r\n");
		}
	}
	return 0;
}

int disable_debug_cmd (U32 argc, char **argv, contextInfo_t *c)
{
	if (c->isDebug == TRUE) {
		cprintf (c, "Disabling debug-mode\r\n");
		c->isDebug = FALSE;
	}
	return 0;
}

int get_dma_ctx_snapshot (U32 argc, char **argv, contextInfo_t *c)
{
	NvmeCtrl *n = &g_NvmeCtrl;

	cprintf (c, "DMA HEAD\n");
	cprintf (c, "idx: %u\n", n->dma.head_idx);
	/*cprintf (c, "%p\n", n->dma.desc_st[n->dma.head_idx].ptr);*/
	/*cprintf (c, "%p\n", n->dma.desc_st[n->dma.head_idx].req_ptr);*/
	/*cprintf (c, "avail: %d\n", n->dma.desc_st[n->dma.head_idx].available);*/
#ifndef SEMAPHORE_LOCK
	cprintf (c, "valid: %d\n", n->dma.desc_st[n->dma.head_idx].valid);
#endif
	cprintf (c, "iop: %d\n", n->dma.io_processor_st);
	cprintf (c, "DMA TAIL\n");
	cprintf (c, "idx: %u\n", n->dma.tail_idx);
	/*cprintf (c, "%p\n", n->dma.desc_st[n->dma.tail_idx].ptr);*/
	/*cprintf (c, "%p\n", n->dma.desc_st[n->dma.tail_idx].req_ptr);*/
	/*cprintf (c, "avail: %d\n", n->dma.desc_st[n->dma.tail_idx].available);*/
#ifndef SEMAPHORE_LOCK
	cprintf (c, "valid: %d\n", n->dma.desc_st[n->dma.tail_idx].valid);
#endif
	cprintf (c, "ioc: %d\n", n->dma.io_completer_st);
	return 0;
}

