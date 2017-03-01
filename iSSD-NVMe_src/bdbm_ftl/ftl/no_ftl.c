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

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/log2.h>

#include "../bdbm_drv.h"
#include "../params.h"
#include "../debug.h"
#include "no_ftl.h"
#include "abm.h"


struct bdbm_ftl_inf_t _ftl_no_ftl = {
	.ptr_private = NULL,
	.create = bdbm_no_ftl_create,
	.destroy = bdbm_no_ftl_destroy,
	.get_free_ppa = bdbm_no_ftl_get_free_ppa,
	.get_ppa = bdbm_no_ftl_get_ppa,
	.map_lpa_to_ppa = bdbm_no_ftl_map_lpa_to_ppa,
	.invalidate_lpa = bdbm_no_ftl_invalidate_lpa,
	.do_gc = bdbm_no_ftl_do_gc,
	.get_segno = NULL,
};

uint32_t bdbm_no_ftl_create (struct bdbm_drv_info* bdi)
{
	return 0;
}

void bdbm_no_ftl_destroy (struct bdbm_drv_info* bdi)
{
}

uint32_t bdbm_no_ftl_get_free_ppa (struct bdbm_drv_info* bdi, uint64_t lpa, struct bdbm_phyaddr_t* ppa)
{
	return bdbm_no_ftl_get_ppa (bdi, lpa, ppa);
}

uint32_t bdbm_no_ftl_get_ppa (struct bdbm_drv_info* bdi, uint64_t lpa, struct bdbm_phyaddr_t* ppa)
{
	struct nand_params* nand_params;
	uint64_t log2_channels;
	uint64_t log2_chips;
	uint64_t log2_blocks;
	uint64_t log2_pages;

	bdbm_bug_on (bdi == NULL);
	
	nand_params = BDBM_GET_NAND_PARAMS (bdi);

	log2_channels = ilog2 (nand_params->nr_channels);
	log2_chips = ilog2 (nand_params->nr_chips_per_channel);
	log2_blocks = ilog2 (nand_params->nr_blocks_per_chip);
	log2_pages = ilog2 (nand_params->nr_pages_per_block); 

	ppa->channel_no = lpa >> (log2_chips + log2_blocks + log2_pages);
	ppa->channel_no = ppa->channel_no & (nand_params->nr_channels - 1);
	ppa->chip_no = lpa >> (log2_blocks + log2_pages);
	ppa->chip_no = ppa->chip_no & (nand_params->nr_chips_per_channel - 1);
	ppa->block_no = lpa >> (log2_pages);
	ppa->block_no = ppa->block_no & (nand_params->nr_blocks_per_chip - 1);
	ppa->page_no = lpa & (nand_params->nr_pages_per_block - 1);

	return 0;
}

uint32_t bdbm_no_ftl_map_lpa_to_ppa (struct bdbm_drv_info* bdi, uint64_t lpa, struct bdbm_phyaddr_t* ptr_phyaddr)
{
	return 0;
}

uint32_t bdbm_no_ftl_invalidate_lpa (struct bdbm_drv_info* bdi, uint64_t lpa, uint64_t len)
{
	return 0;
}

uint32_t bdbm_no_ftl_do_gc (struct bdbm_drv_info* bdi)
{
	return 0;
}

