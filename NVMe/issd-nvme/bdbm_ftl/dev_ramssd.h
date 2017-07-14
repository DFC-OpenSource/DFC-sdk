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

#ifndef _BLUEDBM_DEV_RAMSSD_H
#define _BLUEDBM_DEV_RAMSSD_H
/*
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
*/
#include "bdbm_drv.h"
#include "params.h"
/*#include "utils/time.h" */
#include "types.h"

enum RAMSSD_TIMING {
	RAMSSD_TIMING_DISABLE = 1,
	RAMSSD_TIMING_ENABLE_TASKLET,
	RAMSSD_TIMING_ENABLE_HRTIMER,
};

struct dev_ramssd_punit {
	void* ptr_req;
	int64_t target_elapsed_time_us;
	/*struct bdbm_stopwatch sw; */
};

struct dev_ramssd_info {
	uint8_t is_init; /* 0: not initialized, 1: initialized */
	uint8_t timing_mode;	/* 0: disable timing emulation, 1: enable timing emulation */
	struct nand_params* nand_params;
	void* ptr_ssdram; /* DRAM memory for SSD */
	struct dev_ramssd_punit* ptr_punits;	/* parallel units */
/*	spinlock_t ramssd_lock;*/
	void (*intr_handler) (void*, uint8_t *, uint8_t);
#if 0
	struct tasklet_struct* tasklet;	/* tasklet */
	struct hrtimer hrtimer;	/* hrtimer must be at the end of the structure */
#endif
};

struct dev_ramssd_info* dev_ramssd_create (struct nand_params* ptr_nand_params, uint8_t timing_emul, void (*intr_handler)(void*, uint8_t *, uint8_t));
void dev_ramssd_destroy (struct dev_ramssd_info* ptr_ramssd_info);
uint32_t dev_ramssd_send_cmd (struct dev_ramssd_info* ptr_ramssd_info, struct bdbm_llm_req_t* ptr_llm_req );
void dev_ramssd_summary (struct dev_ramssd_info* ptr_ramssd_info);
uint32_t check_bad_blocks (struct bdbm_llm_req_t* ptr_req,uint16_t channel_no,uint16_t chip_no,uint16_t blk_addr);
void get_lun_target(uint32_t channel_no, uint32_t chip_no, uint8_t *target, uint8_t *lun);

/* for snapshot */
uint32_t dev_ramssd_load (struct dev_ramssd_info* ptr_ramssd_info, const char* fn);
uint32_t dev_ramssd_store (struct dev_ramssd_info* ptr_ramssd_info, const char* fn);

/* some inline functions */
inline static 
uint64_t dev_ramssd_get_page_size_main (struct dev_ramssd_info* ptr_ramssd_info) {
	return ptr_ramssd_info->nand_params->page_main_size;
}

inline static 
uint64_t dev_ramssd_get_page_size_oob (struct dev_ramssd_info* ptr_ramssd_info) {
	return ptr_ramssd_info->nand_params->page_oob_size;
}

inline static 
uint64_t dev_ramssd_get_page_size (struct dev_ramssd_info* ptr_ramssd_info) {
	return ptr_ramssd_info->nand_params->page_main_size +
		ptr_ramssd_info->nand_params->page_oob_size;
}

inline static 
uint64_t dev_ramssd_get_block_size (struct dev_ramssd_info* ptr_ramssd_info) {
	return dev_ramssd_get_page_size (ptr_ramssd_info) *	
		ptr_ramssd_info->nand_params->nr_pages_per_block;
}

inline static 
uint64_t dev_ramssd_get_chip_size (struct dev_ramssd_info* ptr_ramssd_info) {
	return dev_ramssd_get_block_size (ptr_ramssd_info) * 
		ptr_ramssd_info->nand_params->nr_blocks_per_chip;
}

inline static 
uint64_t dev_ramssd_get_channel_size (struct dev_ramssd_info* ptr_ramssd_info) {
	return dev_ramssd_get_chip_size (ptr_ramssd_info) * 
		ptr_ramssd_info->nand_params->nr_chips_per_channel;
}

inline static 
uint64_t dev_ramssd_get_ssd_size (struct dev_ramssd_info* ptr_ramssd_info) {
	return dev_ramssd_get_channel_size (ptr_ramssd_info) * 
		ptr_ramssd_info->nand_params->nr_channels;
}

inline static 
uint64_t dev_ramssd_get_pages_per_block (struct dev_ramssd_info* ptr_ramssd_info) {
	return ptr_ramssd_info->nand_params->nr_pages_per_block;
}

inline static 
uint64_t dev_ramssd_get_blocks_per_chips (struct dev_ramssd_info* ptr_ramssd_info) {
	return ptr_ramssd_info->nand_params->nr_blocks_per_chip;
}

inline static 
uint64_t dev_ramssd_get_chips_per_channel (struct dev_ramssd_info* ptr_ramssd_info) {
	return ptr_ramssd_info->nand_params->nr_chips_per_channel;
}

inline static 
uint64_t dev_ramssd_get_channles_per_ssd (struct dev_ramssd_info* ptr_ramssd_info) {
	return ptr_ramssd_info->nand_params->nr_channels;
}

inline static 
uint64_t dev_ramssd_get_chips_per_ssd (struct dev_ramssd_info* ptr_ramssd_info) {
	return ptr_ramssd_info->nand_params->nr_channels *
		ptr_ramssd_info->nand_params->nr_chips_per_channel;
}

inline static 
uint64_t dev_ramssd_get_blocks_per_ssd (struct dev_ramssd_info* ptr_ramssd_info) {
	return dev_ramssd_get_chips_per_ssd (ptr_ramssd_info) *
		ptr_ramssd_info->nand_params->nr_blocks_per_chip;
}

inline static 
uint64_t dev_ramssd_get_pages_per_ssd (struct dev_ramssd_info* ptr_ramssd_info) {
	return dev_ramssd_get_blocks_per_ssd (ptr_ramssd_info) *
		ptr_ramssd_info->nand_params->nr_pages_per_block;
}

inline static 
uint8_t dev_ramssd_is_init (struct dev_ramssd_info* ptr_ramssd_info) {
	if (ptr_ramssd_info->is_init == 1)
		return 0;
	else 
		return 1;
}

#endif /* _BLUEDBM_DEV_RAMSSD_H */
