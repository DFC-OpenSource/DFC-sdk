#include "nvme.h"
#include "bdbm_ftl/dev_dragonfire.h"
#include "dma_df_nand.h"
#include "uatomic.h"
#include "bdbm_ftl/params.h"

/*TODO to declare the used global variables*/
extern bdbm_dev_private_t _dp;

extern NvmeCtrl g_NvmeCtrl;
unsigned long long int gc_phy_addr[70];
uint8_t* ls2_virt_addr[5];
uint8_t first_dma = 0;
void *df_io_completer (void *arg);

atomic_t act_desc_cnt = ATOMIC_INIT(0);
Nand_DmaCtrl nand_DmaCtrl[NAND_TABLE_COUNT];
Desc_Track Desc_trck[TOTAL_NAND_DESCRIPTORS];

void init_df_dma_mgr (FpgaCtrl *fpga);
void deinit_df_dma_mgr (void);

static inline void __get_lun_target (uint32_t channel_no, uint32_t chip_no, uint8_t *target, uint8_t *lun) 
{
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
}

static inline void reset_df_dma_descriptors (void)
{
	int tab_no, loop;
	NvmeCtrl *n = &g_NvmeCtrl;
	nand_csr_reg csr_reset = {0,0,0,1,1,0};

	for (tab_no = 0; tab_no < NAND_TABLE_COUNT ;tab_no++) {
		memcpy ((void *)n->fpga.nand_dma_regs.csr[tab_no],(void *) &csr_reset, sizeof (nand_csr_reg));
		nand_DmaCtrl[tab_no].tail_idx = nand_DmaCtrl[tab_no].head_idx = 0;
		for (loop =0; loop< (NAND_DESC_PER_TABLE*8) ;loop++) {
			memset((uint32_t*)n->fpga.nand_dma_regs.table[tab_no]+loop, 0,sizeof(uint32_t));
		}
	}
}

static uint32_t df_fill_dma_desc (
		void *req_ptr, 
		struct bdbm_phyaddr_t *phy_addr, 
		uint32_t col_addr,
		uint8_t *host_addr, 
		uint32_t len, 
		uint32_t req_flag) 
{
	uint32_t ret = 0;
	uint8_t loop, lun, target;
	uint64_t l_addr;
	nand_descriptor_t desc = {0};
	NvmeCtrl *n = &g_NvmeCtrl;
	uint8_t tab_id = phy_addr->channel_no;
	uint16_t head_idx = nand_DmaCtrl[tab_id].head_idx;
	static uint32_t desc_head_idx =0; /*to be moved into desc-tracker and reset with dma reset calls -TODO*/
	__get_lun_target (phy_addr->channel_no, phy_addr->chip_no, &target, &lun);
	do {
		if (Desc_trck[desc_head_idx].is_used == TRACK_IDX_FREE) {
			break;
		}
		usleep(1);
	} while (1);

	do {
		mutex_lock (&nand_DmaCtrl[tab_id].DescSt[head_idx].available);
		if (!nand_DmaCtrl[tab_id].DescSt[head_idx].valid) {
			break;
		}
		mutex_unlock (&nand_DmaCtrl[tab_id].DescSt[head_idx].available);
		usleep(1);
	} while(1);

	desc.row_addr = (lun << 21) | ((phy_addr->block_no & 0xFFF) << 9) | (phy_addr->page_no & 0x1FF);

	desc.column_addr = col_addr;

	desc.buffer_id = 0x0;
	desc.channel_id = 0x0;
	desc.target = target;
	desc.length = len;
	if (req_flag & DF_BIT_READ_OP || req_flag & DF_BIT_STATUS_OP) {
		desc.dir = 0x1;
		if (req_flag & DF_BIT_STATUS_OP) {
			desc.command = CMD_READ_STATUS_ENH;
			desc.ecc = DISABLE_ECC;
		} else if (req_flag & DF_BIT_CONFIG_IO) {
			desc.ecc = DISABLE_ECC;
			desc.command = CMD_READ_PARAMETER;
		} else {
			desc.command = CMD_READ;
		}
	} else if (req_flag & DF_BIT_ERASE_OP) {
		desc.ecc = DISABLE_ECC; 
		desc.command = CMD_BLOCK_ERASE;
	} else if (req_flag & DF_BIT_WRITE_OP) {
		if (req_flag & DF_BIT_MGMT_IO) {
			if (len == 0) {
				desc.no_prog = SET_NO_PROG;
				desc.ecc = DISABLE_ECC;
			} else {
				desc.desc_load = SET_LOAD_BIT;
			}
			if (req_flag & DF_BIT_IS_OOB) {
				desc.command = CMD_CHANGE_WRITE_COL;
			} else {
				desc.command = CMD_PAGE_PROG;
			}
		} else {
			if (req_flag & DF_BIT_IS_OOB) {
				desc.command = CMD_CHANGE_WRITE_COL;
				desc.desc_load = SET_LOAD_BIT;
			} else {
				if (req_flag & DF_BIT_PAGE_END) {
					desc.desc_load = SET_LOAD_BIT;
				}
				desc.no_prog = SET_NO_PROG;   /*No_prog = 1*/
				desc.command = CMD_PAGE_PROG;
			}
		}
	} else if (req_flag & DF_BIT_RESET_OP) {
		desc.ecc = DISABLE_ECC;
		desc.no_data = SET_NO_DATA;
		desc.command = CMD_RESET;
	}
		 

	if (((req_flag & DF_BIT_IS_OOB) && (req_flag & DF_BIT_DATA_IO)) || \
		(req_flag & DF_BIT_STATUS_OP) || (req_flag & DF_BIT_SCAN_BB)) {
		if ((req_flag & DF_BIT_WRITE_OP) && !(req_flag & DF_BIT_STATUS_OP)) {
			l_addr = (uint64_t)host_addr;
			/*memcpy(nand_DmaCtrl[tab_id].DescSt[head_idx].virt_oob, \
					(uint8_t *)cmd_buff->data_buffer, NAND_PAGE_OOB_SIZE);*/
			memcpy(nand_DmaCtrl[tab_id].DescSt[head_idx].virt_oob, \
					(uint8_t *)l_addr, NAND_PAGE_OOB_SIZE);
		}
		desc.data_buff_LSB = ((uint64_t)(nand_DmaCtrl[tab_id].DescSt[head_idx].phy_oob) & 0xffffffff);
		desc.data_buff_MSB = ((uint64_t)(nand_DmaCtrl[tab_id].DescSt[head_idx].phy_oob) >> 32);
	} else {
		if (host_addr != NULL) {
			desc.data_buff_LSB = ((uint64_t)(host_addr) & 0xffffffff);
			desc.data_buff_MSB = ((uint64_t)(host_addr) >> 32);
		}
	}
	desc.irq_en = 0;
	desc.hold = 0;
	desc.desc_id = head_idx+1;
	desc.OwnedByfpga = 1;

	nand_DmaCtrl[tab_id].DescSt[head_idx].req_flag = req_flag;
	nand_DmaCtrl[tab_id].DescSt[head_idx].req_ptr = req_ptr;
	nand_DmaCtrl[tab_id].DescSt[head_idx].valid = 1;
#if 0
	for(loop=0; loop<8; loop++) {
		memcpy((uint32_t *)(nand_DmaCtrl[tab_id].DescSt[head_idx].ptr)+loop, \
				(uint32_t*)&desc+loop, sizeof(uint32_t)); 
	}
#endif
	memcpy(nand_DmaCtrl[tab_id].DescSt[head_idx].ptr, \
				&desc, sizeof(nand_descriptor_t)); 
	if(first_dma == 0) {
		for(loop=0; loop<NAND_TABLE_COUNT; loop++) {
			((nand_csr_reg *)(n->fpga.nand_dma_regs.csr[loop]))->start = 1;
			((nand_csr_reg *)(n->fpga.nand_dma_regs.csr[loop]))->loop = 1;
		}
		first_dma++;
	}

	Desc_trck[desc_head_idx].DescSt_ptr = &(nand_DmaCtrl[tab_id].DescSt[head_idx]);
	Desc_trck[desc_head_idx].is_used = TRACK_IDX_USED;

	mutex_unlock (&nand_DmaCtrl[tab_id].DescSt[head_idx].available);
	atomic_inc(&act_desc_cnt);

	CIRCULAR_INCR(desc_head_idx, TOTAL_NAND_DESCRIPTORS);
	CIRCULAR_INCR(nand_DmaCtrl[tab_id].head_idx, NAND_DESC_PER_TABLE);
	return ret;
}

uint32_t df_setup_nand_desc (
		void *req_ptr, 
		struct bdbm_phyaddr_t *phy_addr, 
		uint32_t col_addr,
		uint8_t *host_addr, 
		uint32_t len, 
		uint32_t req_flag) 
{
	uint32_t ret = 0;

	ret = df_fill_dma_desc (req_ptr, phy_addr, col_addr, host_addr, len, req_flag);

#ifdef GET_CMD_STATUS
	if (((req_flag & DF_BIT_IS_OOB) || (req_flag & DF_BIT_ERASE_OP) || \
			(req_flag & DF_BIT_MGMT_IO && req_ptr != NULL)) && !(req_flag & DF_BIT_READ_OP)) {

		ret |= df_fill_dma_desc (req_ptr, phy_addr, 0, NULL, 1, req_flag|DF_BIT_STATUS_OP);
	}
#endif

	return ret; 
}

static void setup_df_desc_context ()
{
	NvmeCtrl *n = &g_NvmeCtrl;
	uint32_t tab_no = 0, desc_no = 0;
	uint32_t *tab_ptr;
	int i = 0;

	for(tab_no=0; tab_no<NAND_TABLE_COUNT; tab_no++){
		tab_ptr = n->fpga.nand_dma_regs.table[tab_no];
		for(desc_no= 0; desc_no< NAND_DESC_PER_TABLE; desc_no++,i++) {
			nand_DmaCtrl[tab_no].DescSt[desc_no].ptr = (uint8_t *)(tab_ptr + (desc_no * NAND_DESC_SIZE));
			nand_DmaCtrl[tab_no].DescSt[desc_no].req_flag = 0;
			nand_DmaCtrl[tab_no].DescSt[desc_no].virt_oob = ls2_virt_addr[0] + (i * 1024);
			nand_DmaCtrl[tab_no].DescSt[desc_no].phy_oob = (uint8_t*)(gc_phy_addr[0] + (i * 1024));
			nand_DmaCtrl[tab_no].DescSt[desc_no].cmdbuf_virt_addr = (uint64_t)(ls2_virt_addr[0] + 0x200000 + (i * CMD_STRUCT_SIZE));
			nand_DmaCtrl[tab_no].DescSt[desc_no].cmdbuf_phy_addr = (gc_phy_addr[0] + 0x200000+ (i * CMD_STRUCT_SIZE));

			nand_DmaCtrl[tab_no].DescSt[desc_no].valid = 0;
			mutex_init (&nand_DmaCtrl[tab_no].DescSt[desc_no].available, NULL);
		}
	}
}

static void df_dma_default (Nand_DmaRegs *regs)
{
	uint8_t tbl;
	for(tbl = 0 ;tbl < NAND_TABLE_COUNT ; tbl++) {
		nand_DmaCtrl[tbl].head_idx=0;
		nand_DmaCtrl[tbl].tail_idx=0; 

		((nand_csr_reg *)regs->csr[tbl])->reset = 1;
		((nand_csr_reg *)regs->csr[tbl])->reset = 0;
	}
	setup_df_desc_context ();
}

void init_df_dma_mgr (FpgaCtrl *fpga)
{
	Nand_DmaRegs *regs = &fpga->nand_dma_regs;
	uint32_t loop;

	for (loop=0; loop<NAND_TABLE_COUNT; loop++) {
		*(regs->table_sz[loop]) =  NAND_DESC_PER_TABLE;
	}

	for (loop = 0; loop < 5; loop++) {
		ls2_virt_addr[loop] = mmap_oob_addr(1024,loop);
	}

	for (loop = 0; loop<5; loop++) {
		if(!ls2_virt_addr[loop]) {
			perror("mmap_oob:");
			return;
		}
	}

	df_dma_default (regs);
}

void deinit_df_dma_mgr (void)
{
	int i;
	for (i=0; i<5; i++) {
		munmap(ls2_virt_addr[i] , (1024*getpagesize()));
	}
}

void *df_io_completer (void *arg)
{
	NvmeCtrl *n = (NvmeCtrl*)arg;
	nand_csf *csf;
	nand_csf csf_bak;
	nand_csr_reg **csr = (nand_csr_reg **)n->fpga.nand_dma_regs.csr;
	uint64_t desc_tail_idx = 0;
	uint8_t io_stat = 0, tbl, tab_id = 0, status = 0;
	uint32_t err,flag, csf_val = 0;

	while (1) {
		do {
			if (Desc_trck[desc_tail_idx].is_used == TRACK_IDX_USED) {
				break;
			}
			usleep(1);
		} while (1);

		do {
			mutex_lock (&(Desc_trck[desc_tail_idx].DescSt_ptr->available));
			if (Desc_trck[desc_tail_idx].DescSt_ptr->valid) {
				break;
			}
			mutex_unlock (&(Desc_trck[desc_tail_idx].DescSt_ptr->available));
			usleep(1);
		}  while (1);

		do {
			csf = (nand_csf *)((uint32_t*)(Desc_trck[desc_tail_idx].DescSt_ptr->ptr) + 7);
			csf_val = *((uint32_t *)csf);
			csf = (nand_csf *)&csf_val;
			usleep(1);
		}while (csf->OwnedByFpga);

		while (csf->OwnedByFpga) {
			usleep(1);
		}

		memcpy (&csf_bak, csf, sizeof(nand_csf));

		if(csf_bak.dma_cmp){
			io_stat = 0;
		} else {
			err = csr[tab_id]->err_code;
			if(err == 1 || err == 2) {  
				printf("READ/WRITE error occured\n");
				io_stat = 1;
			}
		}

		flag = Desc_trck[desc_tail_idx].DescSt_ptr->req_flag;
#ifdef GET_CMD_STATUS
		if (flag & DF_BIT_STATUS_OP || flag & DF_BIT_READ_OP) {
			if (flag & DF_BIT_STATUS_OP) {
				if (flag & DF_BIT_WRITE_OP) {
					if (((*(Desc_trck[desc_tail_idx].DescSt_ptr->virt_oob) & 0x42) == 0x42) && \
							((*(Desc_trck[desc_tail_idx].DescSt_ptr->virt_oob) & 0x21) == 0x21)) {
						io_stat = 1;
						printf("ERROR: write operation fails with status : %x\n", \
								*(Desc_trck[desc_tail_idx].DescSt_ptr->virt_oob));
					}
				} else if (flag & DF_BIT_ERASE_OP) {
					if ((*(Desc_trck[desc_tail_idx].DescSt_ptr->virt_oob) & 0x21) == 0x21) {
						io_stat = 1;
						printf("ERROR: erase operation fails with status : %x\n", \
								*(Desc_trck[desc_tail_idx].DescSt_ptr->virt_oob));
					}
				}
				status |= io_stat;
			}

			if ((flag & DF_BIT_DATA_IO)	|| (flag & DF_BIT_GC_OP)) {
				if ((flag & DF_BIT_IS_OOB) || (flag & DF_BIT_ERASE_OP) || \
						((flag & DF_BIT_DATA_IO) && (flag & DF_BIT_READ_OP))) {
					dm_df_completion_cb (Desc_trck[desc_tail_idx].DescSt_ptr->req_ptr,status);
					status = 0;
					io_stat = 0;
				}
			} else if (flag & DF_BIT_SCAN_BB) {
				bdbm_scan_bb_completion_cb (Desc_trck[desc_tail_idx].DescSt_ptr->req_ptr, \
						Desc_trck[desc_tail_idx].DescSt_ptr->virt_oob, status);
			} else if (flag & DF_BIT_MGMT_IO || flag & DF_BIT_SEM_USED) {
				if (Desc_trck[desc_tail_idx].DescSt_ptr->req_ptr != NULL) {
					sem_post((sem_t*)Desc_trck[desc_tail_idx].DescSt_ptr->req_ptr);
				}
				status = 0;
				io_stat = 0;
			}
		}
#else
		if ((flag & DF_BIT_DATA_IO)	|| (flag & DF_BIT_GC_OP)) {
			if ((flag & DF_BIT_IS_OOB) || (flag & DF_BIT_ERASE_OP) || \
					((flag & DF_BITS_DATA_PAGE) && (flag & DF_BIT_READ_OP))) {
				dm_df_completion_cb (Desc_trck[desc_tail_idx].DescSt_ptr->req_ptr,io_stat);
			}
		} else if (flag & DF_BIT_MGMT_IO || flag & DF_BIT_SEM_USED) {
			sem_post((sem_t*)Desc_trck[desc_tail_idx].DescSt_ptr->req_ptr);
		}
#endif

		Desc_trck[desc_tail_idx].is_used = TRACK_IDX_FREE;
		Desc_trck[desc_tail_idx].DescSt_ptr->valid = 0;
		mutex_unlock (&(Desc_trck[desc_tail_idx].DescSt_ptr->available));

		atomic_dec(&act_desc_cnt);
		CIRCULAR_INCR (desc_tail_idx, TOTAL_NAND_DESCRIPTORS);

		if(io_stat) {
#if 0
			reset_df_dma_descriptors();
			for(tbl =0;tbl <NAND_TABLE_COUNT;tbl++){
				csr[tbl]->reset = 1;
				csr[tbl]->reset = 0;
			}
			first_dma =0;
#endif
		}
	}
}
