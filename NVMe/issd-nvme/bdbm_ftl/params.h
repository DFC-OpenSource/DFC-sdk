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

#ifndef _BLUEDBM_PARAMS_H
#define _BLUEDBM_PARAMS_H
#include "types.h"
/* default parameters for a BlueDBM device */


#define NR_PAGES_PER_BLOCK      		512
#define NR_BLOCKS_PER_CHIP      		2096
#define NR_CHIPS_PER_CHANNEL			4	/*Four LUNs per chip in a NAND DIMM*/
#define NR_CHANNELS				8	/*Four Chips in a NAND DIMM, we are using two dimms*/

/* NAND parameters */
#define NAND_PAGE_SIZE         			16384
/*#define NAND_PAGE_SIZE         			8192*/
/*#define NAND_PAGE_SIZE         			4096*/
/*#define NAND_PAGE_SIZE         			16384*/
//#define NAND_PAGE_OOB_SIZE     			64
#define NAND_PAGE_OOB_SIZE     			1024
#define NAND_OOB_OFFSET					16318
//#define NAND_PAGE_OOB_SIZE     			0
/*#define NR_PAGES_PER_BLOCK      		128*/
/*#define NR_BLOCKS_PER_CHIP      		1024*/
/*#define NR_CHIPS_PER_CHANNEL			8*/
/*#define NR_CHANNELS						8*/

/* 4-bus / 4-chip / 1 GB */
/*
#define NR_PAGES_PER_BLOCK      		128
#define NR_BLOCKS_PER_CHIP      		32*4
#define NR_CHIPS_PER_CHANNEL			4
#define NR_CHANNELS						4
*/
/*15 GB*/
/*
#define NR_PAGES_PER_BLOCK      		240
#define NR_BLOCKS_PER_CHIP      		256
#define NR_CHIPS_PER_CHANNEL			8
#define NR_CHANNELS						8
*/
/*8GB*/
/*
#define NR_PAGES_PER_BLOCK      		256
#define NR_BLOCKS_PER_CHIP      		257
#define NR_CHIPS_PER_CHANNEL			4
#define NR_CHANNELS						8
*/


/*7 GB*/
/*
#define NR_PAGES_PER_BLOCK      		448
#define NR_BLOCKS_PER_CHIP      		128
#define NR_CHIPS_PER_CHANNEL			4
#define NR_CHANNELS						8
*/
#define BLKOFS							256

#define NAND_HOST_BUS_TRANS_TIME_US		0		/* assume to be 0 */
#define NAND_CHIP_BUS_TRANS_TIME_US		100		/* 100us */
#define NAND_PAGE_PROG_TIME_US			1300	/* 1.3ms */	
#define NAND_PAGE_READ_TIME_US			100		/* 100us */
#define NAND_BLOCK_ERASE_TIME_US		3000	/* 3ms */

/* device-type parameters */
#define DEVICE_TYPE_RAMDRIVE       		0
#define DEVICE_TYPE_BLUESIM				1
#define DEVICE_TYPE_BLUEDBM_EMUL		2
#define DEVICE_TYPE_BLUEDBM				3
#define DEVICE_TYPE_DRAGON_FIRE			4


/* default parameters for a device driver */
#define MAPPING_POLICY_NO_FTL			0
#define MAPPING_POLICY_SEGMENT			1
#define MAPPING_POLICY_PAGE         	2

#define GC_POLICY_MERGE             	0
#define GC_POLICY_RAMDOM            	1
#define GC_POLICY_GREEDY            	2
#define GC_POLICY_COST_BENEFIT      	3

#define WL_POLICY_NONE              	0
#define WL_DUAL_POOL                	1

#define QUEUE_NO						0
#define QUEUE_SINGLE_FIFO				1
#define QUEUE_MULTI_FIFO				2

#define KERNEL_SECTOR_SIZE				512		/* kernel sector size is usually set to 512 bytes */

#define TRIM_ENABLE						1		/* 1: enable, 2: disable */
#define TRIM_DISABLE					2

#define HOST_BLOCK						1
#define HOST_DIRECT						2

#define LLM_NO_QUEUE					1
#define LLM_MULTI_QUEUE					2

#define HLM_NO_BUFFER					1
#define HLM_BUFFER						2
#define HLM_RSD							3


/* Parameters for Kernel Modules */
extern int _param_nr_channels;
extern int _param_nr_chips_per_channel;
extern int _param_nr_blocks_per_chip;
extern int _param_nr_pages_per_block;
extern int _param_page_main_size;
extern int _param_page_oob_size;
extern int _param_ssd_type;
extern int _param_kernel_sector_size;	/* 512 Bytes */

extern int _param_mapping_policy;
extern int _param_gc_policy;
extern int _param_wl_policy;
extern int _param_queuing_policy;
extern int _param_trim;


/* parameter structures */
struct driver_params {
	uint32_t mapping_policy;
	uint32_t gc_policy;
	uint32_t wl_policy;
	uint32_t kernel_sector_size;
	uint32_t queueing_policy;
	uint32_t trim;
	uint32_t host_type;
	uint32_t llm_type;
	uint32_t hlm_type;
	uint32_t mapping_type;
};

typedef struct nand_params {
	uint64_t nr_channels; //FPGA
	uint64_t nr_chips_per_channel; //FPGA
	uint64_t nr_blocks_per_chip; // UNFC
	uint64_t nr_pages_per_block; //UNFC
	uint64_t page_main_size; //UNFC
	uint64_t page_oob_size; //UNFC
	uint64_t subpage_size; //UNFC
	uint32_t device_type;//UNFC
	uint8_t timing_mode;
	uint64_t device_capacity_in_byte; //calculated
	uint64_t page_prog_time_us;
	uint64_t page_read_time_us;
	uint64_t block_erase_time_us;

	uint64_t nr_blocks_per_channel; // calculated
	uint64_t nr_blocks_per_ssd;
	uint64_t nr_chips_per_ssd;
	uint64_t nr_pages_per_ssd;
	uint64_t nr_subpages_per_block;
	uint64_t nr_subpages_per_page;
	uint64_t nr_subpages_per_ssd; /* subpage size must be4 KB */
}nand_params_t;

struct bdbm_params {
	struct driver_params driver;
	struct nand_params nand;
};

void display_default_params (void);
struct bdbm_params* read_default_params (void);

#endif /* _BLUEDBM_PARAMS_H */
