#include "bdbm_drv.h"
#include "params.h"
#include "dm_dragonfire.h"
#include "dev_dragonfire.h"
#include "../dma_df_nand.h"
#include "platform.h"

#define PAGE_OFFSET 4
#define PAGE_OFFSET_OPERATOR (PAGE_OFFSET - 1)

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
extern uint32_t df_setup_nand_desc (cmd_info_t *cmd);

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
	uint8_t page_off, req_completed = 0, EN_cmd = 0, EN_deputy_cmd = 0;
	uint8_t nr_kp_per_fp = np->page_main_size >> KERNEL_PAGE_SHIFT; 
	cmd_info_t cmd, deputy_cmd; 

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
		cmd = (cmd_info_t){0};
		deputy_cmd = (cmd_info_t){0};
		EN_cmd = 0;
		EN_deputy_cmd = 0;
		req_completed = 0;
		switch (llm_req->req_type) {
			case REQTYPE_READ:
				cmd.req_info = (void*)llm_req;
				deputy_cmd.req_info = (void*)llm_req;

				bdbm_memcpy (&cmd.phyaddr, llm_req->phyaddr, sizeof(bdbm_phyaddr_t));
				cmd.phyaddr.block_no = cmd.phyaddr.block_no << 1;
				deputy_cmd.phyaddr = cmd.phyaddr;
				deputy_cmd.phyaddr.block_no = deputy_cmd.phyaddr.block_no + 1;

				deputy_cmd.col_addr = 0x0;

				for (page_off=0; page_off < llm_req->nr_subpgs; page_off++) {
					if (llm_req->offset[page_off] < 4) {
						cmd.host_ptr[llm_req->offset[page_off]] = llm_req->pptr_kpgs[page_off];
						cmd.data_len[llm_req->offset[page_off]] = np->subpage_size;
						EN_cmd = 1;
					} else {
						deputy_cmd.host_ptr[llm_req->offset[page_off] & PAGE_OFFSET_OPERATOR] = llm_req->pptr_kpgs[page_off];
						deputy_cmd.data_len[llm_req->offset[page_off] & PAGE_OFFSET_OPERATOR] = np->subpage_size;
					EN_deputy_cmd = 1;
					}
				}
				cmd.req_flag = EN_deputy_cmd ? flag[llm_cnt] | DF_BIT_READ_OP :\
							   flag[llm_cnt] | DF_BIT_READ_OP | DF_BIT_CMD_END;
				
				if (EN_cmd) {
					for (page_off = 0; page_off < 4; page_off++) {
						if (cmd.data_len[page_off]) {
							cmd.col_addr = np->subpage_size * page_off;
							break;
						}
					}
					ret |= df_setup_nand_desc (&cmd);
				}

				if (EN_deputy_cmd) {
					for (page_off = 0; page_off < 4; page_off++) {
						if (deputy_cmd.data_len[page_off]) {
							deputy_cmd.col_addr = np->subpage_size * page_off;
							break;
						}
					}
					deputy_cmd.req_flag = flag[llm_cnt] | DF_BIT_READ_OP | DF_BIT_CMD_END;
					ret |= df_setup_nand_desc (&deputy_cmd);
				}
				break;

			case REQTYPE_GC_READ:
				cmd.req_info = (void*)llm_req;
				deputy_cmd.req_info = (void*)llm_req;

				bdbm_memcpy (&cmd.phyaddr, llm_req->phyaddr, sizeof(bdbm_phyaddr_t));
				cmd.phyaddr.block_no = cmd.phyaddr.block_no << 1;
				deputy_cmd.phyaddr = cmd.phyaddr;
				deputy_cmd.phyaddr.block_no = deputy_cmd.phyaddr.block_no + 1;

				for (page_off=0; page_off < nr_kp_per_fp; page_off++) {
					if (page_off < 4) {
						if (llm_req->kpg_flags[page_off] == MEMFLAG_KMAP_PAGE) {
							cmd.host_ptr[page_off] = llm_req->pptr_kpgs[page_off];
							cmd.data_len[page_off] = np->subpage_size;
							EN_cmd = 1;
						}	
					} else {
						if (llm_req->kpg_flags[page_off] == MEMFLAG_KMAP_PAGE) {
							deputy_cmd.host_ptr[page_off & PAGE_OFFSET_OPERATOR] = llm_req->pptr_kpgs[page_off];
							deputy_cmd.data_len[page_off & PAGE_OFFSET_OPERATOR] = np->subpage_size;
							EN_deputy_cmd = 1;
						}
					}
				}
				cmd.req_flag = EN_deputy_cmd ? flag[llm_cnt] | DF_BIT_READ_OP :\
							   flag[llm_cnt] | DF_BIT_READ_OP | DF_BIT_CMD_END;

				if (EN_cmd) {
					cmd.oob_ptr = llm_req->phy_ptr_oob;
					cmd.oob_len = ACTL_OOB_SIZE;
					ret |= df_setup_nand_desc (&cmd);
				}

				if (EN_deputy_cmd) {
					deputy_cmd.oob_ptr = llm_req->phy_ptr_oob + ACTL_OOB_SIZE;
					deputy_cmd.oob_len = ACTL_OOB_SIZE;
					deputy_cmd.req_flag = flag[llm_cnt] | DF_BIT_READ_OP | DF_BIT_CMD_END;
					ret |= df_setup_nand_desc (&deputy_cmd);
				}
				break;

			case REQTYPE_WRITE:
			case REQTYPE_GC_WRITE:
				cmd.req_info = (void*)llm_req;
				bdbm_memcpy (&cmd.phyaddr, llm_req->phyaddr, sizeof(bdbm_phyaddr_t));
				cmd.phyaddr.block_no = cmd.phyaddr.block_no << 1;
				cmd.col_addr = 0x0;
				for (page_off=0; page_off < (nr_kp_per_fp >> 1); page_off++) {
					cmd.host_ptr[page_off] = llm_req->pptr_kpgs[page_off];
					cmd.data_len[page_off] = np->subpage_size;
					if (llm_req->kpg_flags[page_off + 1] != MEMFLAG_KMAP_PAGE) {
						req_completed = 1;
						break;
					}
				}
				cmd.oob_ptr = (llm_req->req_type == REQTYPE_GC_WRITE) ? \
							  llm_req->phy_ptr_oob : llm_req->ptr_oob;
				cmd.oob_len = ACTL_OOB_SIZE;
				cmd.req_flag = req_completed ? flag[llm_cnt] | DF_BIT_WRITE_OP | \
							   DF_BIT_CMD_END : flag[llm_cnt] | DF_BIT_WRITE_OP;
				cmd.is_Mul_Pln = !req_completed;
				ret |= df_setup_nand_desc (&cmd);


				if (!req_completed) {
					deputy_cmd.req_info = (void*)llm_req;
					bdbm_memcpy (&deputy_cmd.phyaddr, llm_req->phyaddr, \
							sizeof(bdbm_phyaddr_t));
					deputy_cmd.phyaddr.block_no = (deputy_cmd.phyaddr.block_no << 1)+ 1;
					deputy_cmd.col_addr = 0x0;
					for ( ; page_off < nr_kp_per_fp; page_off++) {
						deputy_cmd.host_ptr[page_off & PAGE_OFFSET_OPERATOR] = llm_req->pptr_kpgs[page_off];
						deputy_cmd.data_len[page_off & PAGE_OFFSET_OPERATOR] = np->subpage_size;
						if ((llm_req->kpg_flags[page_off + 1] != MEMFLAG_KMAP_PAGE) || \
								(page_off == (nr_kp_per_fp-1))) {
							break;
						}
					}
					deputy_cmd.oob_ptr = (llm_req->req_type == REQTYPE_GC_WRITE) ? \
								  llm_req->phy_ptr_oob : llm_req->ptr_oob;
					deputy_cmd.oob_ptr += ACTL_OOB_SIZE;
					deputy_cmd.oob_len = ACTL_OOB_SIZE;
					deputy_cmd.req_flag = flag[llm_cnt] | DF_BIT_WRITE_OP | \
										  DF_BIT_CMD_END;
					deputy_cmd.is_Mul_Pln = 0;
					ret |= df_setup_nand_desc (&deputy_cmd);
				}
				break;

			case REQTYPE_GC_ERASE:
				cmd.req_info = (void*)llm_req;
				bdbm_memcpy (&cmd.phyaddr, llm_req->phyaddr, sizeof(bdbm_phyaddr_t));
				cmd.phyaddr.block_no = cmd.phyaddr.block_no << 1;
				cmd.req_flag = flag[llm_cnt] | DF_BIT_ERASE_OP;
				ret |= df_setup_nand_desc (&cmd);

				deputy_cmd.req_info = (void*)llm_req;
				bdbm_memcpy (&deputy_cmd.phyaddr, llm_req->phyaddr, \
						sizeof(bdbm_phyaddr_t));
				deputy_cmd.phyaddr.block_no = (deputy_cmd.phyaddr.block_no << 1)+ 1;
				deputy_cmd.req_flag = flag[llm_cnt] | DF_BIT_ERASE_OP | DF_BIT_CMD_END;
				ret |= df_setup_nand_desc (&deputy_cmd);
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
	bdi->ptr_hlm_inf->end_req (bdi, ptr_llm_req);
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
