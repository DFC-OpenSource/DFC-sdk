#include "bdbm_drv.h"
#include "params.h"
#include "dm_dragonfire.h"
#include "dev_dragonfire.h"
#include "../dma_df_nand.h"
#include "platform.h"

struct bdbm_dm_inf_t _dm_dragonfire_inf = {
	.ptr_private = NULL,
	.probe = dm_df_probe,
	.open = dm_df_open,
	.close = dm_df_close,
	.make_req = dm_df_make_req,
	.end_req = dm_df_end_req,
	.load = dm_df_load,
	.store = dm_df_store,
};

bdbm_dev_private_t _dp;
extern struct bdbm_drv_info* _bdi;

void dm_df_completion_cb (void *req_ptr, uint8_t ret)
{
	struct bdbm_llm_req_t* ptr_llm_req = (struct bdbm_llm_req_t*)req_ptr;

	ptr_llm_req->ret = ret;
	_bdi->ptr_dm_inf->end_req (_bdi, ptr_llm_req);
}


uint32_t dm_df_probe (struct bdbm_drv_info *bdi)
{
	int ret;
	bdi->ptr_dm_inf->ptr_private = (void*)&_dp;
	memcpy((void*)&_dp.nand, (void*)&bdi->ptr_bdbm_params->nand, sizeof(bdbm_device_params_t));
	ret = bdbm_get_dimm_status (&_dp);
	bdbm_bug_on (ret < 0);
	memcpy((void*)&bdi->ptr_bdbm_params->nand, (void*)&_dp.nand, sizeof(bdbm_device_params_t));
	ret = setup_new_ssd_context (&_dp);
	bdbm_bug_on (ret < 0);
	return ret;
}

uint32_t dm_df_open (struct bdbm_drv_info *bdi)
{                                                                                         
	return 0;
}

void dm_df_close (struct bdbm_drv_info *bdi)
{
}

uint32_t dm_df_make_req (struct bdbm_drv_info *bdi, struct bdbm_llm_req_t **pptr_llm_req)
{
	/*TODO to elect the advance command from the basic command*/
	uint32_t *flag = NULL;
	uint32_t ret = 0, llm_cnt, nr_llm_reqs;
	struct nand_params* np = (struct nand_params*)BDBM_GET_NAND_PARAMS(bdi);
	uint64_t hlm_len;
	uint8_t page_off;
	uint8_t nr_kp_per_fp = np->page_main_size/KERNEL_PAGE_SIZE; 

	if (pptr_llm_req[0]->req_type == REQTYPE_GC_READ || \
		pptr_llm_req[0]->req_type == REQTYPE_GC_WRITE || \
		pptr_llm_req[0]->req_type == REQTYPE_GC_ERASE) {
		struct bdbm_hlm_req_gc_t *hlm_req;
		hlm_req = (struct bdbm_hlm_req_gc_t *)(pptr_llm_req[0]->ptr_hlm_req);
		hlm_len = hlm_req->nr_reqs;
		nr_llm_reqs = hlm_req->nr_reqs;
	} else {
		struct bdbm_hlm_req_t *hlm_req;
		hlm_req = (struct bdbm_hlm_req_t *)(pptr_llm_req[0]->ptr_hlm_req);
		hlm_len = hlm_req->len;
		nr_llm_reqs = hlm_req->nr_llm_reqs;
	}
	
	flag = (uint32_t *)bdbm_malloc(sizeof(uint32_t) * nr_llm_reqs);

	if(flag == NULL) {
		bdbm_error("DM Flag memory allocation failed");
		return 1;
	}

	for (llm_cnt = 0; llm_cnt < nr_llm_reqs; llm_cnt++) {
		struct bdbm_llm_req_t *llm_req = pptr_llm_req[llm_cnt];
		switch (llm_req->req_type) {
			case REQTYPE_READ:
			case REQTYPE_WRITE:
				flag[llm_cnt] = DF_BIT_DATA_IO;
				break;
			case REQTYPE_GC_READ:
			case REQTYPE_GC_WRITE:
			case REQTYPE_GC_ERASE:
				flag[llm_cnt] = DF_BIT_GC_OP;
				break;
		}
	}
	for (llm_cnt = 0; llm_cnt < nr_llm_reqs && !ret; llm_cnt++) {
		struct bdbm_llm_req_t *llm_req = pptr_llm_req[llm_cnt];
		switch (llm_req->req_type) {
			case REQTYPE_READ:
				ret = df_setup_nand_rd_desc (
						(void*)llm_req,
						llm_req->phyaddr,
						llm_req->offset * np->subpage_size,
						llm_req->pptr_kpgs[0],
						np->subpage_size,
						flag[llm_cnt] | DF_BIT_IS_PAGE);
				break;
			case REQTYPE_GC_READ:
				for (page_off=0; page_off < nr_kp_per_fp; page_off++) {
					if (llm_req->kpg_flags[page_off] == MEMFLAG_KMAP_PAGE) {
						ret |= df_setup_nand_rd_desc (
								(void*)llm_req,
								llm_req->phyaddr,
								page_off * np->subpage_size,
								llm_req->pptr_kpgs[page_off],
								np->subpage_size,
								flag[llm_cnt] | DF_BIT_IS_PAGE);
					}
				}
				ret |= df_setup_nand_rd_desc (
						(void*)llm_req,
						llm_req->phyaddr,
						(uint32_t)np->page_main_size,
						llm_req->phy_ptr_oob,
						np->page_oob_size,
						flag[llm_cnt] | DF_BIT_IS_OOB);
				break;

			case REQTYPE_WRITE:
				for (page_off=0; page_off < nr_kp_per_fp; page_off++) {
					if ((llm_req->kpg_flags[page_off + 1] != MEMFLAG_KMAP_PAGE) || (page_off == (nr_kp_per_fp-1))) {
						ret |= df_setup_nand_wr_desc (
								(void*)llm_req,
								llm_req->phyaddr,
								page_off * np->subpage_size,
								llm_req->pptr_kpgs[page_off],
								np->subpage_size*(page_off+1),
								flag[llm_cnt] | DF_BIT_IS_PAGE | DF_BIT_PAGE_END);
						break;
					}
					ret |= df_setup_nand_wr_desc (
							(void*)llm_req,
							llm_req->phyaddr,
							page_off * np->subpage_size,
							llm_req->pptr_kpgs[page_off],
							np->subpage_size*(page_off+1),
							flag[llm_cnt] | DF_BIT_IS_PAGE);
				}

				ret |= df_setup_nand_wr_desc (
						(void*)llm_req,
						llm_req->phyaddr,
						(uint32_t)np->page_main_size,
						llm_req->ptr_oob,
						np->page_oob_size,
						flag[llm_cnt] | DF_BIT_IS_OOB);
				break;

			case REQTYPE_GC_WRITE:
				for (page_off=0; page_off < nr_kp_per_fp; page_off++) {
					if ((llm_req->kpg_flags[page_off+1] != MEMFLAG_KMAP_PAGE) || (page_off == (nr_kp_per_fp-1))) {
						ret |= df_setup_nand_wr_desc (
								(void*)llm_req,
								llm_req->phyaddr,
								page_off * np->subpage_size,
								llm_req->pptr_kpgs[page_off],
								np->subpage_size*(page_off+1),
								flag[llm_cnt] | DF_BIT_IS_PAGE | DF_BIT_PAGE_END);
						break;
					}
						
					ret |= df_setup_nand_wr_desc (
							(void*)llm_req,
							llm_req->phyaddr,
							page_off * np->subpage_size,
							llm_req->pptr_kpgs[page_off],
							np->subpage_size*(page_off+1),
							flag[llm_cnt] | DF_BIT_IS_PAGE);
				}

				ret |= df_setup_nand_wr_desc (
						(void*)llm_req,
						llm_req->phyaddr,
						(uint32_t)np->page_main_size,
						llm_req->phy_ptr_oob,
						np->page_oob_size,
						flag[llm_cnt] | DF_BIT_IS_OOB);
				break;

			case REQTYPE_GC_ERASE:
				ret = df_setup_nand_erase_desc (
						(void*)llm_req,
						llm_req->phyaddr,
						0,
						NULL,
						0,
						flag[llm_cnt]);
				break;

			case REQTYPE_READ_DUMMY:
			case REQTYPE_TRIM:
				ret = 0;
				dm_df_completion_cb ((void*)llm_req, 0);
				break;

			default:
				bdbm_error ("invalid command");
				ret = 1;
				break;
		}
	}
	bdbm_free(flag);
	return ret;			
}

void dm_df_end_req (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* ptr_llm_req)
{
	bdbm_bug_on (ptr_llm_req == NULL);
	bdi->ptr_llm_inf->end_req (bdi, ptr_llm_req);
}

uint32_t dm_df_load (struct bdbm_drv_info* bdi, void *pmt, void *bai) 
{
	uint8_t punits,ret;
	bdbm_nblock_st_t *iobb_pools;

	_dp.pmt = (bdbm_pme_t*)pmt;
	_dp.bai = (bdbm_abm_info_t*)bai;

	_dp.bmi[MGMTB_BMT] = (bdbm_nblock_st_t *)_dp.bai->blocks;


	ret = bdbm_ftl_load_from_flash (&_dp);
	iobb_pools = _dp.bmi[MGMTB_IOBB_POOL];

	for (punits = 0; punits < _dp.mgmt_blkptr[MGMTB_IOBB_POOL].nEntries; punits++,iobb_pools++) {
		_dp.bai->punits_iobb_pool[punits] = iobb_pools->block;
	}

	if (_dp.dimm_status != BDBM_ST_LOAD_SUCCESS) {
		ret = 1;
	}

	return ret;
}

uint32_t dm_df_store (struct bdbm_drv_info* bdi)
{
	return bdbm_ftl_store_to_flash (&_dp);
}
