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
#include <linux/slab.h>
#include <linux/interrupt.h>
*/
#include "debug.h"
#include "platform.h"
#include "bdbm_drv.h"
#include "dev_ramssd.h"
#include "syslog.h"
//#include "../nand_dma.h"
/*#include "utils/file.h" */

#include "../nvme.h"
#include "../uatomic.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

static int ramssd_fd = 0;
extern unsigned long long int gc_phy_addr[80];
extern uint8_t* ls2_virt_addr[3];


/* Functions for Managing DRAM SSD */
#ifndef MTL
static uint8_t* __ramssd_page_addr (
	struct dev_ramssd_info* ptr_ramssd_info,
	uint64_t channel_no,
	uint64_t chip_no,
	uint64_t block_no,
	uint64_t page_no)
{
	uint8_t* ptr_ramssd = NULL;
	uint64_t ramssd_addr = 0;

	/* calculate the address offset */
	ramssd_addr += dev_ramssd_get_channel_size (ptr_ramssd_info) * channel_no;
	ramssd_addr += dev_ramssd_get_chip_size (ptr_ramssd_info) * chip_no;
	ramssd_addr += dev_ramssd_get_block_size (ptr_ramssd_info) * block_no;
	ramssd_addr += dev_ramssd_get_page_size (ptr_ramssd_info) * page_no;

	//ramssd_addr = ((channel_no + 1) * (chip_no + 1) * (block_no + 1) * (page_no + 1) * dev_ramssd_get_page_size (ptr_ramssd_info));
	/* get the address */
	ptr_ramssd = (uint8_t*)(ptr_ramssd_info->ptr_ssdram) + ramssd_addr;

	return ptr_ramssd;
}
#else
static uint64_t __ramssd_page_addr (
    struct dev_ramssd_info* ptr_ramssd_info,
    uint64_t channel_no,
    uint64_t chip_no,
    uint64_t block_no,
    uint64_t page_no)
{
    uint64_t ramssd_addr = 0;

    /* calculate the address offset */
    ramssd_addr += dev_ramssd_get_channel_size (ptr_ramssd_info) * channel_no;
    ramssd_addr += dev_ramssd_get_chip_size (ptr_ramssd_info) * chip_no;
    ramssd_addr += dev_ramssd_get_block_size (ptr_ramssd_info) * block_no;
    ramssd_addr += dev_ramssd_get_page_size (ptr_ramssd_info) * page_no;

    //ramssd_addr = ((channel_no + 1) * (chip_no + 1) * (block_no + 1) * (page_no + 1) * dev_ramssd_get_page_size (ptr_ramssd_info));
    /* get the address */

    return ramssd_addr;
}
#endif
static uint8_t* __ramssd_block_addr (
	struct dev_ramssd_info* ptr_ramssd_info,
	uint64_t channel_no,
	uint64_t chip_no,
	uint64_t block_no)
{
	uint8_t* ptr_ramssd = NULL;
	uint64_t ramssd_addr = 0;

	/* calculate the address offset */
	ramssd_addr += dev_ramssd_get_channel_size (ptr_ramssd_info) * channel_no;
	ramssd_addr += dev_ramssd_get_chip_size (ptr_ramssd_info) * chip_no;
	ramssd_addr += dev_ramssd_get_block_size (ptr_ramssd_info) * block_no;

	/* get the address */
	ptr_ramssd = (uint8_t*)(ptr_ramssd_info->ptr_ssdram) + ramssd_addr;

	return ptr_ramssd;
}

static void* __ramssd_alloc_ssdram (struct nand_params* ptr_nand_params)
{
	void* ptr_ramssd = NULL;
	uint64_t page_size_in_bytes;
	uint64_t nr_pages_in_ssd;
	uint64_t ssd_size_in_bytes;

	page_size_in_bytes = 
		ptr_nand_params->page_main_size + 
		ptr_nand_params->page_oob_size;

	nr_pages_in_ssd =
		ptr_nand_params->nr_channels *
		ptr_nand_params->nr_chips_per_channel *
		ptr_nand_params->nr_blocks_per_chip *
		ptr_nand_params->nr_pages_per_block;

	ssd_size_in_bytes = 
		nr_pages_in_ssd * 
		page_size_in_bytes;

	bdbm_msg ("=====================================================================");
	bdbm_msg ("RAM DISK INFO");
	bdbm_msg ("=====================================================================");

	bdbm_msg ("page size (bytes) = %lu (%lu + %lu)", 
		page_size_in_bytes, 
		ptr_nand_params->page_main_size, 
		ptr_nand_params->page_oob_size);

	bdbm_msg ("# of pages in the SSD = %lu", nr_pages_in_ssd);

	bdbm_msg ("the SSD capacity: %lu (B), %lu (KB), %lu (MB)",
		ptr_nand_params->device_capacity_in_byte,
		BDBM_SIZE_KB(ptr_nand_params->device_capacity_in_byte),
		BDBM_SIZE_MB(ptr_nand_params->device_capacity_in_byte));


	/* allocate the memory for the SSD */
	ramssd_fd = open ("/run/ns1_nvme", O_RDWR);
	if (ramssd_fd == -1) {
		perror("../ns1_nvme");
		return NULL;
	}
#define HP_LENGTH (8*4*128*128UL*4160)
	ptr_ramssd = mmap (0, HP_LENGTH, PROT_READ | PROT_WRITE, MAP_SHARED, ramssd_fd, 0);
	if (ptr_ramssd == (void *)-1) {
		perror("drive_fd");
		close (ramssd_fd);
		ramssd_fd = 0;
		return ptr_ramssd = NULL;
	}
	printf ("nvme drive fd mmap Returned address is %p\n", ptr_ramssd);
// 	bdbm_memset ((uint8_t*)ptr_ramssd, 0xFF, ssd_size_in_bytes * sizeof (uint8_t));

	bdbm_msg ("ramssd addr = %p", ptr_ramssd);
	bdbm_msg ("");

	/* good; return ramssd addr */
	return (void*)ptr_ramssd;
}

static void __ramssd_free_ssdram (void* ptr_ramssd) 
{
#ifndef MTL
	munmap (ptr_ramssd, HP_LENGTH);
	ptr_ramssd = NULL;
#endif
	close (ramssd_fd);
	ramssd_fd = 0;
}
#ifndef MTL
static uint8_t __ramssd_read_page (
	struct dev_ramssd_info* ptr_ramssd_info, 
	uint64_t channel_no,
	uint64_t chip_no,
	uint64_t block_no,
	uint64_t page_no,
	uint8_t* kpg_flags,
	uint8_t** ptr_page_data,
	uint8_t* ptr_oob_data,
	uint8_t oob,
	uint8_t partial,
	uint16_t offset)
{
	uint8_t ret = 0;
	uint8_t* ptr_ramssd_addr = NULL;
	uint32_t nr_pages, loop;

	/* get the memory address for the destined page */
	if ((ptr_ramssd_addr = __ramssd_page_addr (
			ptr_ramssd_info, channel_no, chip_no, block_no, page_no)) == NULL) {
		bdbm_error ("invalid ram_addr (%p)", ptr_ramssd_addr);
		ret = 1;
		goto fail;
	}

	/* for better performance, RAMSSD directly copies the SSD data to kernel pages */
	nr_pages = ptr_ramssd_info->nand_params->page_main_size / KERNEL_PAGE_SIZE;
	if (ptr_ramssd_info->nand_params->page_main_size % KERNEL_PAGE_SIZE != 0) {
		bdbm_error ("The page-cache granularity (%d) is not matched to the flash page size (%lu)", 
			KERNEL_PAGE_SIZE, ptr_ramssd_info->nand_params->page_main_size);
		ret = 1;
		goto fail;
	}

	/* copy the main page data to a buffer */
	for (loop = 0; loop < nr_pages; loop++) {
		if (partial == 1 && kpg_flags[loop] == MEMFLAG_KMAP_PAGE) {
			continue;
		}
		if (kpg_flags != NULL && (kpg_flags[loop] & MEMFLAG_DONE) == MEMFLAG_DONE) {
			/* it would be possible that part of the page was already read at the level of the cache */
			continue;
		}
		void *dest;
		dest = bdbm_memcpy (
			ptr_page_data[loop], 
			ptr_ramssd_addr + KERNEL_PAGE_SIZE * loop + offset, 
			KERNEL_PAGE_SIZE-offset
		);
	}

	/* copy the OOB data to a buffer */
	if (partial == 0 && oob && ptr_oob_data != NULL) {
		bdbm_memcpy (
			ptr_oob_data, 
			ptr_ramssd_addr + ptr_ramssd_info->nand_params->page_main_size,
			ptr_ramssd_info->nand_params->page_oob_size
		);
	}

fail:
	return ret;
}

static uint8_t __ramssd_prog_page (
	struct dev_ramssd_info* ptr_ramssd_info, 
	uint64_t channel_no,
	uint64_t chip_no,
	uint64_t block_no,
	uint64_t page_no,
	uint8_t* kpg_flags,
	uint8_t** ptr_page_data,
	uint8_t* ptr_oob_data,
	uint8_t oob,
	uint16_t offset)
{
	uint8_t ret = 0;
	uint8_t* ptr_ramssd_addr = NULL;
	uint32_t nr_pages, loop;

	/* get the memory address for the destined page */
	if ((ptr_ramssd_addr = __ramssd_page_addr (
			ptr_ramssd_info, channel_no, chip_no, block_no, page_no)) == NULL) {
		bdbm_error ("invalid ram addr (%p)", ptr_ramssd_addr);
		ret = 1;
		goto fail;
	}

	/* for better performance, RAMSSD directly copies the SSD data to pages */
	nr_pages = ptr_ramssd_info->nand_params->page_main_size / KERNEL_PAGE_SIZE;
	if (ptr_ramssd_info->nand_params->page_main_size % KERNEL_PAGE_SIZE != 0) {
		bdbm_error ("The page-cache granularity (%d) is not matched to the flash page size (%lu)", 
			KERNEL_PAGE_SIZE, ptr_ramssd_info->nand_params->page_main_size);
		ret = 1;
		goto fail;
	}

	/* copy the main page data to a buffer */
	for (loop = 0; loop < nr_pages; loop++) {
		bdbm_memcpy (
			ptr_ramssd_addr + KERNEL_PAGE_SIZE * loop + offset, 
			ptr_page_data[loop], 
			KERNEL_PAGE_SIZE-offset
		);
	}

	/* copy the OOB data to a buffer */
	if (oob && ptr_oob_data != NULL) {
		bdbm_memcpy (
			ptr_ramssd_addr + ptr_ramssd_info->nand_params->page_main_size,
			ptr_oob_data,
			ptr_ramssd_info->nand_params->page_oob_size
		);
		/*
		bdbm_msg ("lpa: %lu %lu", 
			((uint64_t*)(ptr_ramssd_addr + ptr_ramssd_info->nand_params->page_main_size))[0],
			((uint64_t*)ptr_oob_data)[0]);
		*/
	}

fail:
	return ret;
}

static uint8_t __ramssd_erase_block (
		struct dev_ramssd_info* ptr_ramssd_info, 
		uint64_t channel_no,
		uint64_t chip_no,
		uint64_t block_no)
{
	uint8_t* ptr_ram_addr = NULL;
	/* get the memory address for the destined block */
	if ((ptr_ram_addr = __ramssd_block_addr 
				(ptr_ramssd_info, channel_no, chip_no, block_no)) == NULL) {
		bdbm_error ("invalid ssdram addr (%p)", ptr_ram_addr);
		return 1;
	}

	/* erase the block (set all the values to '1') */
	memset (ptr_ram_addr, 0xFF, dev_ramssd_get_block_size (ptr_ramssd_info));
}
#else

static uint8_t __ramssd_rw_page (struct dev_ramssd_info* ptr_ramssd_info,
		struct bdbm_llm_req_t* ptr_req,
		uint8_t oob,
		uint8_t partial
		)
{
	uint8_t ret = 0;
	uint8_t* ptr_ramssd_addr = NULL;
	uint32_t nr_pages, loop;
	uint64_t ramssd_addr =0;

	uint64_t channel_no=ptr_req->phyaddr->channel_no;
	uint64_t chip_no=ptr_req->phyaddr->chip_no;
	uint64_t block_no=ptr_req->phyaddr->block_no;
	uint64_t page_no=ptr_req->phyaddr->page_no;
	uint8_t* kpg_flags=ptr_req->kpg_flags;
	uint8_t** ptr_page_data=ptr_req->pptr_kpgs;
	uint8_t* ptr_oob_data=(uint8_t*)ptr_req->ptr_oob;
	uint16_t offset=ptr_req->offset;
	uint8_t rw_type = 1, oob_trans = 0;
	uint8_t req_type;

	/* get the memory address for the destined page */
#if 0
	if ((ramssd_addr = __ramssd_page_addr (
					ptr_ramssd_info, channel_no, chip_no, block_no, page_no)) < 0) {
		bdbm_error ("invalid ram_addr (%lu)", ramssd_addr);
		ret = 1;
		goto fail;
	}
#endif
	/* for better performance, RAMSSD directly copies the SSD data to kernel pages */
	nr_pages = ptr_ramssd_info->nand_params->page_main_size / KERNEL_PAGE_SIZE;
	if (ptr_ramssd_info->nand_params->page_main_size % KERNEL_PAGE_SIZE != 0) {
		bdbm_error ("The page-cache granularity (%d) is not matched to the flash page size (%lu)",
				KERNEL_PAGE_SIZE, ptr_ramssd_info->nand_params->page_main_size);
		ret = 1;
		goto fail;
	}
	nand_cmd_struct *cmd_addr = (nand_cmd_struct *)malloc(sizeof(nand_cmd_struct));
	memset(cmd_addr,0,sizeof(nand_cmd_struct));
	uint8_t lun=0, target=0;
	cmd_addr->buffer_channel_id = 0x0;
	cmd_addr->reserv1 = 0x0;
	cmd_addr->reserv2 = 0x0;
	cmd_addr->reserv3 = 0x0;
	get_lun_target(channel_no, chip_no, &target, &lun);
	/* copy the main page data to a buffer */
 //syslog(LOG_INFO,"req_t:%d ch%d cp%d ta%d lu%d bl%d pg%d\n", ptr_req->req_type, channel_no, chip_no, target, lun, block_no, page_no);
	for (loop = 0; loop < nr_pages; loop++) {
		if(ptr_req->req_type == REQTYPE_READ || ptr_req->req_type == REQTYPE_RMW_READ || ptr_req->req_type == REQTYPE_GC_READ || ptr_req->req_type == REQTYPE_LOAD) {
			if (partial == 1 && kpg_flags[loop] == MEMFLAG_KMAP_PAGE) {
				continue;
			}
			if (kpg_flags != NULL && (kpg_flags[loop] & MEMFLAG_DONE) == MEMFLAG_DONE) {
				/* it would be possible that part of the page was already read at the level of the cache */
				continue;
			}
			req_type = FTL_IO;
			cmd_addr->row_addr = (lun << 21) | ((block_no & 0xFFF) << 9) | (page_no & 0x1FF);
			cmd_addr->column_addr = 0x0;
			cmd_addr->len_target_sel = (0x1000 << 16) | target;
			cmd_addr->direction_control = 0x1;	/*read=1*/
			cmd_addr->control = (ECC_EN << 31)/*ECC disable=1*/;
			cmd_addr->command_sel = NAND_READ;
			cmd_addr->data_buffer_LSB = ((uint64_t)(ptr_page_data[loop])) & 0xFFFFFFFF;
			cmd_addr->data_buffer_MSB = ((uint64_t)(ptr_page_data[loop])) >> 32;
			if(ptr_req->req_type == REQTYPE_LOAD){
				req_type = FTL_PMT;
			}
			ret = make_desc(channel_no, ptr_req, req_type, cmd_addr);

		}else if(ptr_req->req_type == REQTYPE_WRITE || ptr_req->req_type == REQTYPE_GC_WRITE || ptr_req->req_type == REQTYPE_STORE){
			

			req_type = FTL_IO;
			cmd_addr->row_addr = (lun << 21) | ((block_no & 0xFFF) << 9) | (page_no & 0x1FF);
			cmd_addr->column_addr = 0x0;
			cmd_addr->len_target_sel = (0x1000 << 16) | target;
			cmd_addr->control = (ECC_EN << 31) | (1 << 5)/*ECC disable=1 | no_prog=1*/;
			cmd_addr->direction_control = 0x0;	/*write=0*/
			cmd_addr->command_sel = NAND_PROG_PAGE;
			cmd_addr->data_buffer_LSB = ((uint64_t)ptr_page_data[loop]) & 0xFFFFFFFF;
			cmd_addr->data_buffer_MSB = ((uint64_t)(ptr_page_data[loop])) >> 32;
			if(ptr_req->req_type == REQTYPE_STORE){
				req_type = FTL_PMT;
				cmd_addr->control = (ECC_EN << 31) | (0 << 5)/*ECC disable=1 | no_prog=1*/;
			}
			ret = make_desc(channel_no, ptr_req, req_type, cmd_addr);
		}
#ifdef CMD_STATUS_DESC
		if (ptr_req->req_type == REQTYPE_READ || ptr_req->req_type == REQTYPE_RMW_READ || ptr_req->req_type == REQTYPE_GC_READ || ptr_req->req_type == REQTYPE_LOAD || ptr_req->req_type == REQTYPE_STORE){
			req_type = NAND_RD_STS;
			memset(cmd_addr,0,sizeof(nand_cmd_struct));
			cmd_addr->row_addr = (lun << 21) | ((block_no & 0xFFF) << 9);
			cmd_addr->column_addr = 0x0;
			cmd_addr->len_target_sel = (0x1 << 16) | target;
			cmd_addr->direction_control = 0x1;	/*read=1*/
			cmd_addr->control = (ECC_DIS << 31)/*ECC disable=1*/;
			cmd_addr->command_sel = NAND_READ_STS_EN;
			cmd_addr->data_buffer_LSB = 0;
			cmd_addr->data_buffer_MSB = 0;
			if ((ptr_req->req_type == REQTYPE_LOAD) || (ptr_req->req_type == REQTYPE_STORE)){	
				req_type = NAND_CMD_STS;
			}
			ret = make_desc(channel_no, ptr_req, req_type, cmd_addr);
		}
#endif
		//ret = make_desc(ptr_page_data[loop],(ramssd_addr + KERNEL_PAGE_SIZE * loop + offset),\
		(KERNEL_PAGE_SIZE-offset),ptr_req,rw_type,0,req_type);
	}
	/* copy the OOB data to a buffer */
	if(ptr_req->req_type == REQTYPE_GC_READ || ptr_req->req_type == REQTYPE_READ) {
		if (partial == 0 && oob && ptr_oob_data != NULL) {
			req_type = FTL_RD_OOB;
			memset(cmd_addr,0,sizeof(nand_cmd_struct));
			cmd_addr->row_addr = (lun << 21) | ((block_no & 0xFFF) << 9) | (page_no & 0x1FF);
			cmd_addr->column_addr = 0x4000;
			cmd_addr->len_target_sel = (0x400 << 16) | target;
			cmd_addr->direction_control = 0x1;	/*read=1*/
			cmd_addr->control = (ECC_EN << 31)/*ECC disable=1*/;
			cmd_addr->command_sel = NAND_READ;
			cmd_addr->data_buffer_LSB = ((uint64_t)ptr_oob_data) & 0xFFFFFFFF;
			cmd_addr->data_buffer_MSB = ((uint64_t)(ptr_oob_data)) >> 32;
			ret = make_desc(channel_no, ptr_req, req_type, cmd_addr);

#ifdef CMD_STATUS_DESC
			req_type = NAND_CMD_STS;
			memset(cmd_addr,0,sizeof(nand_cmd_struct));
			cmd_addr->row_addr = (lun << 21) | ((block_no & 0xFFF) << 9);
			cmd_addr->column_addr = 0x0;
			cmd_addr->len_target_sel = (0x1 << 16) | target;
			cmd_addr->direction_control = 0x1;	/*read=1*/
			cmd_addr->control = (ECC_DIS << 31)/*ECC disable=1*/;
			cmd_addr->command_sel = NAND_READ_STS_EN;
			cmd_addr->data_buffer_LSB = 0;
			cmd_addr->data_buffer_MSB = 0;
			ret = make_desc(channel_no, ptr_req, req_type, cmd_addr);
#endif
		}
	}else if(ptr_req->req_type == REQTYPE_WRITE || ptr_req->req_type == REQTYPE_GC_WRITE){
		if (oob && ptr_oob_data != NULL) {
			req_type = FTL_WR_OOB;
			memset(cmd_addr,0,sizeof(nand_cmd_struct));
			cmd_addr->row_addr = (lun << 21) | ((block_no & 0xFFF) << 9) | (page_no & 0x1FF);
			cmd_addr->column_addr = 0x4000;
			cmd_addr->len_target_sel = (0x400 << 16) | target;
			cmd_addr->direction_control = 0x0;	/*read=1*/
			cmd_addr->control = (ECC_EN << 31) | (0 << 5)/*ECC disable=1 no_prog=0*/;
			cmd_addr->command_sel = NAND_COL_PROG_PAGE;
			cmd_addr->data_buffer_LSB = ((uint64_t)ptr_oob_data) & 0xFFFFFFFF;
			cmd_addr->data_buffer_MSB = ((uint64_t)(ptr_oob_data)) >> 32;
			ret = make_desc(channel_no, ptr_req, req_type, cmd_addr);

#ifdef CMD_STATUS_DESC
			req_type = NAND_CMD_STS;
			memset(cmd_addr,0,sizeof(nand_cmd_struct));
			cmd_addr->row_addr = (lun << 21) | ((block_no & 0xFFF) << 9);
			cmd_addr->column_addr = 0x0;
			cmd_addr->len_target_sel = (0x1 << 16) | target;
			cmd_addr->direction_control = 0x1;	/*read=1*/
			cmd_addr->control = (ECC_DIS << 31)/*ECC disable=1*/;
			cmd_addr->command_sel = NAND_READ_STS_EN;
			cmd_addr->data_buffer_LSB = 0;
			cmd_addr->data_buffer_MSB = 0;
			ret = make_desc(channel_no, ptr_req, req_type, cmd_addr);
#endif

			//ret = make_desc(ptr_oob_data,(ramssd_addr + ptr_ramssd_info->nand_params->page_main_size),\
					ptr_ramssd_info->nand_params->page_oob_size,ptr_req,rw_type,0,oob_trans);
		}
	}
	free(cmd_addr);
fail:
	return ret;
}

static uint8_t __ramssd_erase_block (
		struct dev_ramssd_info* ptr_ramssd_info, 
		struct bdbm_llm_req_t* ptr_req,
		uint64_t channel_no,
		uint64_t chip_no,
		uint64_t block_no)
{
	uint8_t req_type;
	uint8_t lun=0, target=0;
	get_lun_target(channel_no, chip_no, &target, &lun);
	nand_cmd_struct *cmd_addr = (nand_cmd_struct *)malloc(sizeof(nand_cmd_struct));
	memset(cmd_addr,0,sizeof(nand_cmd_struct));
	req_type = FTL_ERASE;
	cmd_addr->row_addr = (lun << 21) | ((block_no & 0xFFF) << 9);
	cmd_addr->column_addr = 0x0;
	cmd_addr->len_target_sel = target;
	cmd_addr->direction_control = 0x0;
	cmd_addr->control = (ECC_DIS << 31);
	cmd_addr->command_sel = NAND_ERASE;
	cmd_addr->data_buffer_LSB = 0x0;
	cmd_addr->data_buffer_MSB = 0x0;
	make_desc(channel_no, ptr_req, req_type, cmd_addr);
	memset(cmd_addr,0,sizeof(nand_cmd_struct));
	//Read Status Enhanced
#ifdef CMD_STATUS_DESC
	req_type = NAND_CMD_STS; 
	cmd_addr->row_addr = (lun << 21) | ((block_no & 0xFFF) << 9);
	cmd_addr->column_addr = 0x0;
	cmd_addr->len_target_sel = (0x1 << 16) | target;
	cmd_addr->direction_control = 0x1;
	cmd_addr->control = (ECC_DIS << 31);
	cmd_addr->command_sel = NAND_READ_STS_EN;
	cmd_addr->data_buffer_LSB = 0;
	cmd_addr->data_buffer_MSB = 0;
	make_desc(channel_no, ptr_req, req_type, cmd_addr);
#endif

	return 0;
}
#endif

static uint32_t __ramssd_send_cmd (
		struct dev_ramssd_info* ptr_ramssd_info, struct bdbm_llm_req_t* ptr_req)
{
	uint8_t ret = 0;
	uint8_t use_oob = 1;	/* read or program OOB by default; why not??? */
	uint8_t use_partial = 0;
	if (ptr_ramssd_info->nand_params->page_oob_size == 0)
		use_oob = 0;

	switch (ptr_req->req_type) {
		case REQTYPE_RMW_READ:
			use_partial = 1;
		case REQTYPE_READ:
		case REQTYPE_GC_READ:
#ifdef PMT_ON_FLASH
		case REQTYPE_LOAD:
#endif
#ifndef MTL
		ret = __ramssd_read_page (
			ptr_ramssd_info, 
			ptr_req->phyaddr->channel_no, 
			ptr_req->phyaddr->chip_no, 
			ptr_req->phyaddr->block_no, 
			ptr_req->phyaddr->page_no, 
			ptr_req->kpg_flags,
			ptr_req->pptr_kpgs,
			ptr_req->ptr_oob,
			use_oob,
			use_partial,
			ptr_req->offset);
#else
		 ret = __ramssd_rw_page (ptr_ramssd_info, ptr_req, use_oob, use_partial);
#endif
		break;

	case REQTYPE_WRITE:
	case REQTYPE_GC_WRITE:
	case REQTYPE_RMW_WRITE:
#ifdef PMT_ON_FLASH
	case REQTYPE_STORE:
#endif
#ifndef MTL
		ret = __ramssd_prog_page (
			ptr_ramssd_info, 
			ptr_req->phyaddr->channel_no,
			ptr_req->phyaddr->chip_no,
			ptr_req->phyaddr->block_no,
			ptr_req->phyaddr->page_no,
			ptr_req->kpg_flags,
			ptr_req->pptr_kpgs,
			ptr_req->ptr_oob,
			use_oob,
			ptr_req->offset);
#else	
	ret = __ramssd_rw_page (ptr_ramssd_info, ptr_req, use_oob, use_partial);
#endif
		break;

	case REQTYPE_GC_ERASE:
#ifndef MTL
		ret = __ramssd_erase_block (
			ptr_ramssd_info, 
			ptr_req->phyaddr->channel_no, 
			ptr_req->phyaddr->chip_no, 
			ptr_req->phyaddr->block_no);
#else
		ret = __ramssd_erase_block (
			ptr_ramssd_info,
			ptr_req,
			ptr_req->phyaddr->channel_no, 
			ptr_req->phyaddr->chip_no, 
			ptr_req->phyaddr->block_no);
#endif
		break;

	case REQTYPE_READ_DUMMY:
		/* do nothing for READ_DUMMY */
		ret = 0;
		break;

	case REQTYPE_TRIM:
		/* do nothing for TRIM */
		ret = 0;
		break;

	case REQTYPE_BAD_BLK_CHK:
		ret = check_bad_blocks(
				ptr_req,
				ptr_req->phyaddr->channel_no,
				ptr_req->phyaddr->chip_no,
				ptr_req->phyaddr->block_no);
		break;	

	default:
		bdbm_error ("invalid command");
		ret = 1;
		break;
	}

	return ret;
}

void __ramssd_cmd_done (struct dev_ramssd_info* ptr_ramssd_info)
{
	uint64_t loop, nr_parallel_units;

	nr_parallel_units = dev_ramssd_get_chips_per_ssd (ptr_ramssd_info);

	for (loop = 0; loop < nr_parallel_units; loop++) {
		unsigned long flags;

		/*spin_lock_irqsave (&ptr_ramssd_info->ramssd_lock, flags); */
		if (ptr_ramssd_info->ptr_punits[loop].ptr_req != NULL) {
			struct dev_ramssd_punit* punit;
			int64_t elapsed_time_in_us;

			punit = &ptr_ramssd_info->ptr_punits[loop];
/*			elapsed_time_in_us = bdbm_stopwatch_get_elapsed_time_us (&punit->sw); */
			elapsed_time_in_us = 0;
			if (elapsed_time_in_us >= punit->target_elapsed_time_us) {
				void* ptr_req = punit->ptr_req;
				punit->ptr_req = NULL;
				/*spin_unlock_irqrestore (&ptr_ramssd_info->ramssd_lock, flags); */

				/* call the interrupt handler */
				ptr_ramssd_info->intr_handler (ptr_req, NULL, 0);
			} else {
/*				spin_unlock_irqrestore (&ptr_ramssd_info->ramssd_lock, flags); */
			}
		} else {
/*			spin_unlock_irqrestore (&ptr_ramssd_info->ramssd_lock, flags); */
		}
	}
}

extern atomic_t act_desc_cnt;
uint32_t nand_reset(){
	uint8_t req_type=NAND_RESET;
	nand_cmd_struct *cmd_addr = (nand_cmd_struct *)malloc(sizeof(nand_cmd_struct));
	uint16_t chan_no = 0;
	uint32_t ret = 0;
	for (chan_no=0; chan_no<NR_CHANNELS; chan_no++) {
		cmd_addr->row_addr = 0xabcd;
		cmd_addr->column_addr = 0x1234;
		cmd_addr->len_target_sel = 0x0;
		cmd_addr->direction_control = 0x0;
		cmd_addr->control = 0x80000000;
		cmd_addr->command_sel = 0x1;
		cmd_addr->data_buffer_LSB = 0x0;
		cmd_addr->data_buffer_MSB = 0x0;
		make_desc(chan_no, NULL,req_type,cmd_addr);
		memset(cmd_addr,0,sizeof(nand_cmd_struct));
		//Read Status

#ifdef CMD_STATUS_DESC
		req_type=NAND_RD_STS;
		cmd_addr->row_addr = 0x0;
		cmd_addr->column_addr = 0x0;
		cmd_addr->len_target_sel = 0x10000;
		cmd_addr->direction_control = 0x1;
		cmd_addr->control = 0x80000000;
		cmd_addr->command_sel = 0x14;
		cmd_addr->data_buffer_LSB = gc_phy_addr[2] & 0xFFFFFFFF;
		cmd_addr->data_buffer_MSB = gc_phy_addr[2] >> 32;
		make_desc(chan_no,NULL,req_type,cmd_addr);
		memset(cmd_addr,0,sizeof(nand_cmd_struct));
#endif
		while (atomic_read(&act_desc_cnt)) {
			usleep(1);
		}
#ifdef CMD_STATUS_DESC
		ret |= *ls2_virt_addr[2];
#endif
	}
	free(cmd_addr);
	return ret;
}
uint32_t check_bad_blocks (struct bdbm_llm_req_t* ptr_req,uint16_t channel_no,uint16_t chip_no,uint16_t blk_addr)
{
	nand_cmd_struct *cmd_structure = (nand_cmd_struct *)malloc(sizeof(nand_cmd_struct));
	uint32_t len =4,page_addr =0,ecc =1;
	uint64_t blk_nr;
	uint16_t host_addr = 2;
	uint8_t pln = 0, lun = 0,trgt = 0,chip = 0,req_type=DM_BAD_BLK;
	blk_nr = (NR_CHIPS_PER_CHANNEL* NR_BLOCKS_PER_CHIP * channel_no +
			NR_BLOCKS_PER_CHIP * chip_no + blk_addr);
	get_lun_target(channel_no, chip_no, &trgt, &lun);
	cmd_structure->row_addr = (lun << 21) | ((blk_addr & 0xFFF) << 9) | (page_addr & 0x1FF);
	cmd_structure->column_addr = 0x4000;
	cmd_structure->len_target_sel = (len << 16) | trgt;
	cmd_structure->direction_control = 1;
	cmd_structure->control = (ecc << 31)/*ECC disable=1*/;
	cmd_structure->command_sel = 0x1e;
	cmd_structure->data_buffer_LSB = ((gc_phy_addr[host_addr] + (blk_nr*4)) & 0xFFFFFFFF);
	cmd_structure->data_buffer_MSB = ((gc_phy_addr[host_addr] + (blk_nr*4)) >> 32);
	make_desc(channel_no,ptr_req,req_type,cmd_structure);
	free(cmd_structure);
	return 0;
}
void get_lun_target(uint32_t channel_no, uint32_t chip_no, uint8_t *target, uint8_t *lun){

	switch(chip_no){
	case 0:
		*target=0; *lun=0;
		break;
	case 1:
		*target=0; *lun=1;
		break;
	case 2:
		*target=1; *lun=0;
		break;
	case 3:
		*target=1; *lun=1;
		break;
	default:
		printf("%s wrong chip and channel num: %d %d\n", __func__, chip_no, channel_no);
	}
	return;
}
#if 0
/* Functions for Timing Management */
static void __ramssd_timing_cmd_done (unsigned long arg)
{
	/* forward it to ramssd_cmd_done */
	__ramssd_cmd_done ((struct dev_ramssd_info*)arg);
}

static enum hrtimer_restart __ramssd_timing_hrtimer_cmd_done (struct hrtimer *ptr_hrtimer)
{
	ktime_t ktime;
	struct dev_ramssd_info* ptr_ramssd_info;
	
	ptr_ramssd_info = (struct dev_ramssd_info*)container_of 
		(ptr_hrtimer, struct dev_ramssd_info, hrtimer);

	/* call a tasklet */
	tasklet_schedule (ptr_ramssd_info->tasklet); 

	ktime = ktime_set (0, 50 * 1000);
	hrtimer_start (&ptr_ramssd_info->hrtimer, ktime, HRTIMER_MODE_REL);

	return HRTIMER_NORESTART;
}

uint32_t __ramssd_timing_register_schedule (struct dev_ramssd_info* ptr_ramssd_info)
{
	switch (ptr_ramssd_info->timing_mode) {
	case RAMSSD_TIMING_DISABLE:
		__ramssd_cmd_done (ptr_ramssd_info);
		break;
	case RAMSSD_TIMING_ENABLE_HRTIMER:
		/*__ramssd_cmd_done (ptr_ramssd_info);*/
		break;
	case RAMSSD_TIMING_ENABLE_TASKLET:
		tasklet_schedule (ptr_ramssd_info->tasklet); 
		break;
	default:
		__ramssd_timing_cmd_done ((unsigned long)ptr_ramssd_info);
		break;
	}

	return 0;
}

uint32_t __ramssd_timing_create (struct dev_ramssd_info* ptr_ramssd_info) 
{
	uint32_t ret = 0;

	switch (ptr_ramssd_info->timing_mode) {
	case RAMSSD_TIMING_DISABLE:
		bdbm_msg ("use TIMING_DISABLE mode!");
		break;
	case RAMSSD_TIMING_ENABLE_HRTIMER: 
		{
			ktime_t ktime;
			bdbm_msg ("HRTIMER is created!");
			hrtimer_init (&ptr_ramssd_info->hrtimer, CLOCK_REALTIME, HRTIMER_MODE_REL);
			ptr_ramssd_info->hrtimer.function = __ramssd_timing_hrtimer_cmd_done;
			ktime = ktime_set (0, 500 * 1000);
			hrtimer_start (&ptr_ramssd_info->hrtimer, ktime, HRTIMER_MODE_REL);
		}
		/* no break! we initialize a tasklet together */
	case RAMSSD_TIMING_ENABLE_TASKLET: 
		bdbm_msg ("TASKLET is created!");
		if ((ptr_ramssd_info->tasklet = (struct tasklet_struct*)
				bdbm_malloc_atomic (sizeof (struct tasklet_struct))) == NULL) {
			bdbm_msg ("bdbm_malloc_atomic failed");
			ret = 1;
		} else {
			tasklet_init (ptr_ramssd_info->tasklet, 
				__ramssd_timing_cmd_done, (unsigned long)ptr_ramssd_info);
		}
		break;
	default:
		bdbm_error ("invalid timing mode");
		ret = 1;
		break;
	}

	return ret;
}

void __ramssd_timing_destory (struct dev_ramssd_info* ptr_ramssd_info)
{
	switch (ptr_ramssd_info->timing_mode) {
	case RAMSSD_TIMING_DISABLE:
		bdbm_msg ("TIMING_DISABLE is done!");
		break;
	case RAMSSD_TIMING_ENABLE_HRTIMER:
		bdbm_msg ("HRTIMER is canceled");
		hrtimer_cancel (&ptr_ramssd_info->hrtimer);
		/* no break! we destroy a tasklet */
	case RAMSSD_TIMING_ENABLE_TASKLET:
		bdbm_msg ("TASKLET is killed");
		tasklet_kill (ptr_ramssd_info->tasklet);
		break;
	default:
		break;
	}
}
#endif
/* Functions Exposed to External Files */
struct dev_ramssd_info* dev_ramssd_create (
	struct nand_params* ptr_nand_params, 
	uint8_t timing_mode,
	void (*intr_handler)(void*,uint8_t *, uint8_t))
{
	uint64_t loop, nr_parallel_units;
	struct dev_ramssd_info* ptr_ramssd_info = NULL;

	/* create a ramssd info */
	if ((ptr_ramssd_info = (struct dev_ramssd_info*)
			bdbm_malloc_atomic (sizeof (struct dev_ramssd_info))) == NULL) {
		bdbm_error ("bdbm_malloc_atomic failed");
		goto fail;
	}

	/* seup parameters */
	ptr_ramssd_info->intr_handler = intr_handler;
	ptr_ramssd_info->timing_mode = timing_mode;
	ptr_ramssd_info->nand_params = ptr_nand_params;
#ifndef MTL
	/* allocate ssdram space */
	if ((ptr_ramssd_info->ptr_ssdram = 
			__ramssd_alloc_ssdram (ptr_ramssd_info->nand_params)) == NULL) {
		bdbm_error ("__ramssd_alloc_ssdram failed");
		goto fail_ssdram;
	}
#endif

	/* create parallel units */
	nr_parallel_units = dev_ramssd_get_chips_per_ssd (ptr_ramssd_info);

	if ((ptr_ramssd_info->ptr_punits = (struct dev_ramssd_punit*)
			bdbm_malloc_atomic (sizeof (struct dev_ramssd_punit) * nr_parallel_units)) == NULL) {
		bdbm_error ("bdbm_malloc_atomic failed");
		goto fail_punits;
	}
	for (loop = 0; loop < nr_parallel_units; loop++) {
		ptr_ramssd_info->ptr_punits[loop].ptr_req = NULL;
	}
#if 0
	/* create and register a tasklet */
	if (__ramssd_timing_create (ptr_ramssd_info) != 0) {
		bdbm_msg ("dev_ramssd_timing_create failed");
		goto fail_timing;
	}
#endif
	/* create spin_lock */
/*	spin_lock_init (&ptr_ramssd_info->ramssd_lock); */

	/* done */
	ptr_ramssd_info->is_init = 1;

	return ptr_ramssd_info;

fail_timing:
	bdbm_free_atomic (ptr_ramssd_info->ptr_punits);

fail_punits:
	__ramssd_free_ssdram (ptr_ramssd_info->ptr_ssdram);

fail_ssdram:
	bdbm_free_atomic (ptr_ramssd_info);

fail:
	return NULL;
}

void dev_ramssd_destroy (struct dev_ramssd_info* ptr_ramssd_info)
{
#if 0
	/* kill tasklet */
	__ramssd_timing_destory (ptr_ramssd_info);
#endif
	/* free ssdram */
	__ramssd_free_ssdram (ptr_ramssd_info->ptr_ssdram);

	/* release other stuff */
	bdbm_free_atomic (ptr_ramssd_info->ptr_punits);
	bdbm_free_atomic (ptr_ramssd_info);
}

uint32_t dev_ramssd_send_cmd (struct dev_ramssd_info* ptr_ramssd_info, struct bdbm_llm_req_t* llm_req)
{
	uint32_t ret;
	// static int ch =0;
    //if(ch ==0) {
      //  ch++;
    //}


	if ((ret = __ramssd_send_cmd (ptr_ramssd_info, llm_req)) == 0) {
#if 0
		unsigned long flags;
		int64_t target_elapsed_time_us = 0;
		uint64_t punit_id;
		/* get the punit_id */
		punit_id = dev_ramssd_get_chips_per_channel (ptr_ramssd_info) * 
			llm_req->phyaddr->channel_no + 
			llm_req->phyaddr->chip_no;

		/* get the target elapsed time depending on the type of req */
		if (ptr_ramssd_info->timing_mode == RAMSSD_TIMING_ENABLE_HRTIMER) {
			switch (llm_req->req_type) {
			case REQTYPE_WRITE:
			case REQTYPE_GC_WRITE:
			case REQTYPE_RMW_WRITE:
				target_elapsed_time_us = ptr_ramssd_info->nand_params->page_prog_time_us;
				break;
			case REQTYPE_READ:
			case REQTYPE_GC_READ:
			case REQTYPE_RMW_READ:
				target_elapsed_time_us = ptr_ramssd_info->nand_params->page_read_time_us;
				break;
			case REQTYPE_GC_ERASE:
				target_elapsed_time_us = ptr_ramssd_info->nand_params->block_erase_time_us;
				break;
			case REQTYPE_READ_DUMMY:
				target_elapsed_time_us = 0;	/* dummy read */
				break;
			default:
				bdbm_error ("invalid REQTYPE (%u)", llm_req->req_type);
				bdbm_bug_on (1);
				break;
			}
			if (target_elapsed_time_us > 0) {
				target_elapsed_time_us -= (target_elapsed_time_us / 10);
			}
		} else {
			target_elapsed_time_us = 0;
		}

		/* register reqs */
		/*spin_lock_irqsave (&ptr_ramssd_info->ramssd_lock, flags); */
		if (ptr_ramssd_info->ptr_punits[punit_id].ptr_req == NULL) {
			ptr_ramssd_info->ptr_punits[punit_id].ptr_req = (void*)llm_req;
			/*ptr_ramssd_info->ptr_punits[punit_id].sw = bdbm_stopwatch_start ();*/
			/*bdbm_stopwatch_start (&ptr_ramssd_info->ptr_punits[punit_id].sw); */
			ptr_ramssd_info->ptr_punits[punit_id].target_elapsed_time_us = target_elapsed_time_us;
		} else {
			bdbm_error ("More than two requests are assigned to the same parallel unit (ptr=%p, punit=%lu, lpa=%lu)",
				ptr_ramssd_info->ptr_punits[punit_id].ptr_req,
				punit_id,
				llm_req->lpa);
/*			spin_unlock_irqrestore (&ptr_ramssd_info->ramssd_lock, flags); */
			ret = 1;
			goto fail;
		}
	/*	spin_unlock_irqrestore (&ptr_ramssd_info->ramssd_lock, flags); */
#if 0
		/* register reqs for callback */
		__ramssd_timing_register_schedule (ptr_ramssd_info);
#endif
#endif
	}

fail:
	return ret;
}

void dev_ramssd_summary (struct dev_ramssd_info* ptr_ramssd_info)
{
	if (ptr_ramssd_info->is_init == 0) {
		bdbm_msg ("RAMSSD is not initialized yet");
		return;
	}

	bdbm_msg ("* A summary of the RAMSSD organization *");
	bdbm_msg (" - Total SSD size: %lu B (%lu MB)", dev_ramssd_get_ssd_size (ptr_ramssd_info), BDBM_SIZE_MB (dev_ramssd_get_ssd_size (ptr_ramssd_info)));
	bdbm_msg (" - Flash chip size: %lu", dev_ramssd_get_chip_size (ptr_ramssd_info));
	bdbm_msg (" - Flash block size: %lu", dev_ramssd_get_block_size (ptr_ramssd_info));
	bdbm_msg (" - Flash page size: %lu (main: %lu + oob: %lu)", dev_ramssd_get_page_size (ptr_ramssd_info), dev_ramssd_get_page_size_main (ptr_ramssd_info), dev_ramssd_get_page_size_oob (ptr_ramssd_info));
	bdbm_msg ("");
	bdbm_msg (" - # of pages per block: %lu", dev_ramssd_get_pages_per_block (ptr_ramssd_info));
	bdbm_msg (" - # of blocks per chip: %lu", dev_ramssd_get_blocks_per_chips (ptr_ramssd_info));
	bdbm_msg (" - # of chips per channel: %lu", dev_ramssd_get_chips_per_channel (ptr_ramssd_info));
	bdbm_msg (" - # of channels: %lu", dev_ramssd_get_channles_per_ssd (ptr_ramssd_info));
	bdbm_msg ("");
	bdbm_msg (" - kernel page size: %d", KERNEL_PAGE_SIZE);
	bdbm_msg ("");
}

/* for snapshot */
uint32_t dev_ramssd_load (struct dev_ramssd_info* ptr_ramssd_info, const char* fn)
{

	//struct file* fp = NULL;
	FILE *fp = NULL;
	uint64_t len = 0;

	bdbm_msg ("dev_ramssd_load - begin");

	if (ptr_ramssd_info->ptr_ssdram == NULL) {
		bdbm_error ("ptr_ssdram is NULL");
		return 1;
	}
	
	//if ((fp = bdbm_fopen (fn, O_RDWR, 0777)) == NULL) {
	if ((fp = fopen (fn,"r")) == NULL ) {
		bdbm_error ("bdbm_fopen failed");
		return 1;
	}

	bdbm_msg ("dev_ramssd_load: DRAM read starts = %lu", len);
	len = dev_ramssd_get_ssd_size (ptr_ramssd_info);
	//len = bdbm_fread (fp, 0, (uint8_t*)ptr_ramssd_info->ptr_ssdram, len);
	len = fread((uint8_t*)ptr_ramssd_info->ptr_ssdram, 1, len, fp);
	bdbm_msg ("dev_ramssd_load: DRAM read ends = %lu", len);

	//bdbm_fclose (fp);
	fclose(fp);

	bdbm_msg ("dev_ramssd_load - done");

	return 0;
}

uint32_t dev_ramssd_store (struct dev_ramssd_info* ptr_ramssd_info, const char* fn)
{

//	struct file* fp = NULL;
//	uint64_t pos = 0;
	FILE *fp = NULL;
	uint64_t len = 0;

	//bdbm_msg ("dev_ramssd_store - begin");

	if (ptr_ramssd_info->ptr_ssdram == NULL) {
		bdbm_error ("ptr_ssdram is NULL");
		return 1;
	}
	
	//if ((fp = bdbm_fopen (fn, O_CREAT | O_WRONLY, 0777)) == NULL) {
	if ((fp = fopen (fn, "w")) == NULL) {
		bdbm_error ("bdbm_fopen failed");
		return 1;
	}

	len = dev_ramssd_get_ssd_size (ptr_ramssd_info);
	//bdbm_msg ("dev_ramssd_store: DRAM store starts = %lu", len);
	/*while (pos < len) {
		pos += bdbm_fwrite (fp, pos, (uint8_t*)ptr_ramssd_info->ptr_ssdram + pos, len - pos);
	}*/
	fwrite((uint8_t*)ptr_ramssd_info->ptr_ssdram, 1, len, fp);
	//bdbm_fsync (fp);
	//bdbm_fclose (fp);
	fclose(fp);

	//bdbm_msg ("dev_ramssd_store: DRAM store ends = %lu", pos);
	bdbm_msg ("dev_ramssd_store - end");

	return 0;
}

