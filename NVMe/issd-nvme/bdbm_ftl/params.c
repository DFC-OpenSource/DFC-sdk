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
#include <linux/moduleparam.h>
#include <linux/slab.h>
*/
#include <syslog.h>
#include "params.h"
#include "platform.h"
#include "debug.h"
#include "bdbm_drv.h"
#include "dev_ramssd.h"
#include "types.h"
#include "list.h"

int _param_nr_channels 				= NR_CHANNELS;
int _param_nr_chips_per_channel		= NR_CHIPS_PER_CHANNEL;
int _param_nr_blocks_per_chip 		= NR_BLOCKS_PER_CHIP;
int _param_nr_pages_per_block 		= NR_PAGES_PER_BLOCK;
int _param_page_main_size 			= NAND_PAGE_SIZE;
int _param_page_oob_size 			= NAND_PAGE_OOB_SIZE;
int _param_subpage_size 			= KERNEL_PAGE_SIZE;
/*int _param_device_type 				= DEVICE_TYPE_RAMDRIVE;*/
/*int _param_device_type 				= DEVICE_TYPE_BLUESIM;*/
/*int _param_device_type 				= DEVICE_TYPE_BLUEDBM_EMUL;*/
/*int _param_device_type 				= DEVICE_TYPE_BLUEDBM;*/
int _param_host_bus_trans_time_us	= NAND_HOST_BUS_TRANS_TIME_US;
int _param_chip_bus_trans_time_us	= NAND_CHIP_BUS_TRANS_TIME_US;
int _param_page_prog_time_us		= NAND_PAGE_PROG_TIME_US; 		
int _param_page_read_time_us		= NAND_PAGE_READ_TIME_US;
int _param_block_erase_time_us		= NAND_BLOCK_ERASE_TIME_US;

int _param_ramdrv_timing_mode   	= RAMSSD_TIMING_DISABLE;
/*int _param_ramdrv_timing_mode   	= RAMSSD_TIMING_ENABLE_TASKLET;*/
/*int _param_ramdrv_timing_mode   	= RAMSSD_TIMING_ENABLE_HRTIMER;*/

int _param_kernel_sector_size		= KERNEL_SECTOR_SIZE;	/* 512 Bytes */
/*int _param_mapping_policy 			= MAPPING_POLICY_PAGE;*/
/*int _param_mapping_policy 			= MAPPING_POLICY_SEGMENT;*/
int _param_gc_policy 				= GC_POLICY_GREEDY;
int _param_wl_policy 				= WL_POLICY_NONE;
int _param_queuing_policy			= QUEUE_NO;
//int _param_trim						= TRIM_ENABLE;
int _param_trim						= TRIM_DISABLE;
int _param_host_type				= HOST_BLOCK;
/*int _param_hlm_type					= HLM_BUFFER;*/
/*int _param_hlm_type					= HLM_NO_BUFFER;*/
/*int _param_hlm_type					= HLM_RSD;*/
/*int _param_llm_type					= LLM_NO_QUEUE;*/
int _param_llm_type				= LLM_NO_QUEUE;

/*#define USE_RISA*/
#ifdef USE_RISA
int _param_mapping_policy 			= MAPPING_POLICY_SEGMENT;
int _param_hlm_type					= HLM_RSD;
#else
int _param_mapping_policy 			= MAPPING_POLICY_PAGE;
int _param_hlm_type					= HLM_NO_BUFFER;
#endif


#ifdef USE_DRAM
int _param_device_type 				= DEVICE_TYPE_RAMDRIVE;
#else
int _param_device_type 				= DEVICE_TYPE_DRAGON_FIRE;
//int _param_device_type 			= DEVICE_TYPE_BLUEDBM;
#endif
#if 0	
module_param (_param_nr_channels, int, 0000);
module_param (_param_nr_chips_per_channel, int, 0000);
module_param (_param_nr_blocks_per_chip, int, 0000);
module_param (_param_nr_pages_per_block, int, 0000);
module_param (_param_page_main_size, int, 0000);
module_param (_param_page_oob_size, int, 0000);
module_param (_param_device_type, int, 0000);
module_param (_param_mapping_policy, int, 0000);
module_param (_param_gc_policy, int, 0000);
module_param (_param_wl_policy, int, 0000);
module_param (_param_queuing_policy, int, 0000);
module_param (_param_kernel_sector_size, int, 0000);
module_param (_param_trim, int, 0000);
module_param (_param_host_type, int, 0000);
module_param (_param_llm_type, int, 0000);
module_param (_param_hlm_type, int, 0000);
module_param (_param_ramdrv_timing_mode, int, 0000);
module_param (_param_host_bus_trans_time_us, int, 0000);
module_param (_param_chip_bus_trans_time_us, int, 0000);
module_param (_param_page_prog_time_us, int, 0000);
module_param (_param_page_read_time_us, int, 0000);
module_param (_param_block_erase_time_us, int, 0000);

MODULE_PARM_DESC (_param_nr_channels, "# of channels");
MODULE_PARM_DESC (_param_nr_chips_per_channel, "# of chips per channel");
MODULE_PARM_DESC (_param_nr_blocks_per_chip, "# of blocks per chip");
MODULE_PARM_DESC (_param_nr_pages_per_block, "# of pages per block");
MODULE_PARM_DESC (_param_page_main_size, "page main size");
MODULE_PARM_DESC (_param_page_oob_size, "page oob size");
MODULE_PARM_DESC (_param_mapping_policy, "mapping policy");
MODULE_PARM_DESC (_param_gc_policy, "garbage collection policy");
MODULE_PARM_DESC (_param_wl_policy, "wear-leveling policy");
MODULE_PARM_DESC (_param_queuing_policy, "queueing policy");
MODULE_PARM_DESC (_param_kernel_sector_size, "kernel sector size");
MODULE_PARM_DESC (_param_trim, "trim option");
MODULE_PARM_DESC (_param_host_type, "host interface type");
MODULE_PARM_DESC (_param_llm_type, "low-level memory management type");
MODULE_PARM_DESC (_param_hlm_type, "high-level memory management type");
MODULE_PARM_DESC (_param_ramdrv_timing_mode, "timing mode for ramdrive");
MODULE_PARM_DESC (_param_host_bus_trans_time_us, "host bus transfer time");
MODULE_PARM_DESC (_param_chip_bus_trans_time_us, "NAND bus transfer time");
MODULE_PARM_DESC (_param_page_prog_time_us, "page program time");
MODULE_PARM_DESC (_param_page_read_time_us, "page read time");
MODULE_PARM_DESC (_param_block_erase_time_us, "block erasure time");

#endif
void display_default_params (void)
{
	syslog(LOG_INFO,"=====================================================================");
	syslog(LOG_INFO,"DRIVER CONFIGURATION");
	syslog(LOG_INFO,"=====================================================================");
	syslog(LOG_INFO,"mapping policy = %d (0: no ftl, 1: block-mapping, 2: page-mapping)", _param_mapping_policy);
	syslog(LOG_INFO,"gc policy = %d (1: merge 2: random, 3: greedy, 4: cost-benefit)", _param_gc_policy);
	syslog(LOG_INFO,"wl policy = %d (1: none, 2: swap)", _param_wl_policy);
	syslog(LOG_INFO,"trim mode = %d (1: enable, 2: disable)", _param_trim);
	syslog(LOG_INFO,"host type = %d (1: block I/O, 2: direct)", _param_host_type);
	syslog(LOG_INFO,"kernel sector = %d bytes", _param_kernel_sector_size);
	syslog(LOG_INFO,"");

	syslog(LOG_INFO,"=====================================================================");
	syslog(LOG_INFO,"DEVICE PARAMETERS");
	syslog(LOG_INFO,"=====================================================================");
	syslog(LOG_INFO,"# of channels = %d", _param_nr_channels);
	syslog(LOG_INFO,"# of chips per channel = %d", _param_nr_chips_per_channel);
	syslog(LOG_INFO,"# of blocks per chip = %d", _param_nr_blocks_per_chip);
	syslog(LOG_INFO,"# of pages per block = %d", _param_nr_pages_per_block);
	syslog(LOG_INFO,"page main size  = %d bytes", _param_page_main_size);
	syslog(LOG_INFO,"page oob size = %d bytes", _param_page_oob_size);
	syslog(LOG_INFO,"SSD type = %d (0: ramdrive, 1: ramdrive with timing , 2: BlueDBM(emul), 3: BlueDBM)", _param_device_type);
	syslog(LOG_INFO,"");
}

/*int read_default_params (struct bdbm_drv_info* bdi)*/
struct bdbm_params* read_default_params (void)
{
	struct bdbm_params* ptr_params = NULL;

	/* display default parameters */
	display_default_params ();

	/* allocate the memory for parameters */
	if ((ptr_params = (struct bdbm_params*)bdbm_malloc (sizeof (struct bdbm_params))) == NULL) {
		bdbm_error ("failed to allocate the memory for ptr_params");
		return NULL;
	}

	/* handling the module parameters from the command line */
	ptr_params->nand.nr_channels = _param_nr_channels;
	ptr_params->nand.nr_chips_per_channel = _param_nr_chips_per_channel;
	ptr_params->nand.nr_blocks_per_chip = _param_nr_blocks_per_chip;
	ptr_params->nand.nr_pages_per_block = _param_nr_pages_per_block;
	ptr_params->nand.page_main_size = _param_page_main_size;
	ptr_params->nand.page_oob_size = _param_page_oob_size;
	ptr_params->nand.subpage_size = _param_subpage_size;
	ptr_params->nand.device_type = _param_device_type;
	ptr_params->nand.timing_mode = _param_ramdrv_timing_mode;
	ptr_params->nand.page_read_time_us = _param_host_bus_trans_time_us + _param_chip_bus_trans_time_us + _param_page_read_time_us;
	ptr_params->nand.page_prog_time_us = _param_host_bus_trans_time_us + _param_chip_bus_trans_time_us + _param_page_prog_time_us;
	ptr_params->nand.block_erase_time_us = _param_block_erase_time_us;
	ptr_params->nand.nr_blocks_per_channel = 
		ptr_params->nand.nr_chips_per_channel * 
		ptr_params->nand.nr_blocks_per_chip;
	ptr_params->nand.nr_blocks_per_ssd = 
		ptr_params->nand.nr_channels * 
		ptr_params->nand.nr_chips_per_channel * 
		ptr_params->nand.nr_blocks_per_chip;
	ptr_params->nand.nr_chips_per_ssd =
		ptr_params->nand.nr_channels * 
		ptr_params->nand.nr_chips_per_channel;
	ptr_params->nand.nr_pages_per_ssd =
		ptr_params->nand.nr_pages_per_block * 
		ptr_params->nand.nr_blocks_per_ssd;

	ptr_params->nand.nr_subpages_per_page =
		(ptr_params->nand.page_main_size / KERNEL_PAGE_SIZE);
	ptr_params->nand.nr_subpages_per_block = ptr_params->nand.nr_subpages_per_page * 
		ptr_params->nand.nr_pages_per_block;
	ptr_params->nand.nr_subpages_per_ssd = ptr_params->nand.nr_subpages_per_page *
		ptr_params->nand.nr_pages_per_ssd;


	/* calculate the total SSD capacity */
	ptr_params->nand.device_capacity_in_byte = 0;
	ptr_params->nand.device_capacity_in_byte += ptr_params->nand.nr_channels;
	ptr_params->nand.device_capacity_in_byte *= ptr_params->nand.nr_chips_per_channel;
#if 0
	ptr_params->nand.device_capacity_in_byte *= ptr_params->nand.nr_blocks_per_chip;
#endif

	if (_param_mapping_policy == MAPPING_POLICY_PAGE) {
		ptr_params->nand.device_capacity_in_byte *= 
			(ptr_params->nand.nr_blocks_per_chip - ptr_params->nand.nr_blocks_per_chip * 0.06);	/* for FTL overprivisioning */
		syslog(LOG_INFO,"FTL and bad-block overprovisioning: %lu %lu", 
				ptr_params->nand.nr_blocks_per_chip,
				(uint64_t)(ptr_params->nand.nr_blocks_per_chip * 0.06));
	} else {
		ptr_params->nand.device_capacity_in_byte *= 
			(ptr_params->nand.nr_blocks_per_chip - ptr_params->nand.nr_blocks_per_chip * 0.01);	/* for bad-block overprivisioning */
		syslog(LOG_INFO,"Bad-block overprovisioning: %lu %lu", 
				ptr_params->nand.nr_blocks_per_chip,
				(uint64_t)(ptr_params->nand.nr_blocks_per_chip * 0.01));
	}
	ptr_params->nand.device_capacity_in_byte *= ptr_params->nand.nr_pages_per_block;
	ptr_params->nand.device_capacity_in_byte *= ptr_params->nand.page_main_size;

	/* setup driver parameters */
	ptr_params->driver.mapping_policy = _param_mapping_policy;
	ptr_params->driver.gc_policy = _param_gc_policy;
	ptr_params->driver.wl_policy = _param_wl_policy;
	ptr_params->driver.kernel_sector_size = _param_kernel_sector_size;
	ptr_params->driver.trim = _param_trim;
	ptr_params->driver.host_type = _param_host_type; 
	ptr_params->driver.llm_type = _param_llm_type;
	ptr_params->driver.hlm_type = _param_hlm_type;
	ptr_params->driver.mapping_type = _param_mapping_policy;

	return ptr_params;
}

