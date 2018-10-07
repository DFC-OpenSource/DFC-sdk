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

#ifndef _BLUEDBM_FTL_BLOCKFTL_H
#define _BLUEDBM_FTL_BLOCKFTL_H

extern struct bdbm_ftl_inf_t _ftl_block_ftl;

uint32_t bdbm_block_ftl_create (struct bdbm_drv_info* bdi);
void bdbm_block_ftl_destroy (struct bdbm_drv_info* bdi);
uint32_t bdbm_block_ftl_get_free_ppa (struct bdbm_drv_info* bdi, uint64_t lpa, struct bdbm_phyaddr_t* ppa);
uint32_t bdbm_block_ftl_get_ppa (struct bdbm_drv_info* bdi, uint64_t lpa, struct bdbm_phyaddr_t* ppa);
uint32_t bdbm_block_ftl_map_lpa_to_ppa (struct bdbm_drv_info* bdi, uint64_t lpa, struct bdbm_phyaddr_t* ptr_phyaddr);
uint32_t bdbm_block_ftl_invalidate_lpa (struct bdbm_drv_info* bdi, uint64_t lpa, uint64_t len);
uint8_t bdbm_block_ftl_is_gc_needed (struct bdbm_drv_info* bdi);	
uint32_t bdbm_block_ftl_do_gc (struct bdbm_drv_info* bdi);
uint64_t bdbm_block_ftl_get_segno (struct bdbm_drv_info* bdi, uint64_t lpa);
uint32_t bdbm_block_ftl_badblock_scan (struct bdbm_drv_info* bdi);
uint32_t bdbm_block_ftl_load (struct bdbm_drv_info* bdi, const char* fn);
uint32_t bdbm_block_ftl_store (struct bdbm_drv_info* bdi, const char* fn);


#endif /* _BLUEDBM_FTL_BLOCKFTL_H */

