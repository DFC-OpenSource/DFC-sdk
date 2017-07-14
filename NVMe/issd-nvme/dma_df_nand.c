#include "nvme.h"
#include "bdbm_ftl/dev_dragonfire.h"
#include "dma_df_nand.h"
#include "uatomic.h"
#include "bdbm_ftl/params.h"

/*#define NAND_MSI*/

/*TODO to declare the used global variables*/
extern bdbm_dev_private_t _dp;

extern NvmeCtrl g_NvmeCtrl;
unsigned long long int gc_phy_addr[80];
uint8_t* ls2_virt_addr[5];
uint8_t first_dma = 0,CHIP_COUNT=0;
void *df_io_completer (void *arg);
FILE *fp;

atomic_t act_desc_cnt = ATOMIC_INIT(0);
Nand_DmaCtrl nand_DmaCtrl[NAND_TABLE_COUNT];
Desc_Track Desc_trck[TOTAL_NAND_DESCRIPTORS];

void init_df_dma_mgr (FpgaCtrl *fpga);
void deinit_df_dma_mgr (void);
uint32_t df_setup_nand_desc (cmd_info_t *cmd_info);

static inline void __get_lun_target (uint32_t chip_no, uint8_t *target, uint8_t *lun) 
{
	switch (chip_no) {
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
		printf("%s wrong chip num: %d \n", __func__, chip_no);
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

static uint32_t df_fill_dma_desc (cmd_info_t *cmd) 
{
	uint32_t ret = 0;int word =0;
	uint8_t loop, lun, target;
	/*uint64_t l_addr, addr = 0;*/
	nand_descriptor_t desc = {0};
	NvmeCtrl *n = &g_NvmeCtrl;
	uint8_t tab_id = cmd->phyaddr.channel_no;
	uint16_t head_idx = nand_DmaCtrl[tab_id].head_idx;
	uint32_t req_flag = cmd->req_flag;
	static uint32_t desc_head_idx = 0; /*to be moved into desc-tracker and reset with dma reset calls -TODO*/
	__get_lun_target (cmd->phyaddr.chip_no, &target, &lun);
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
		//usleep(1);
	} while(1);

	desc.row_addr = (lun << 21) | ((cmd->phyaddr.block_no & 0xFFF) << 9) | \
		(cmd->phyaddr.page_no & 0x1FF);

	desc.column_addr = cmd->col_addr;
	desc.target = target;
	desc.buffer_id = 0x0;
	desc.channel_id = 0x0;
	desc.data_len_1 = cmd->data_len[0];
	desc.data_len_2 = cmd->data_len[1];
	desc.data_len_3 = cmd->data_len[2];
	desc.data_len_4 = cmd->data_len[3];
	desc.oob_data_len = cmd->oob_len;

	if (cmd->data_len[0]) {
		if (cmd->host_ptr[0]) {
			desc.data_buff_1_LSB = ((uint64_t)(cmd->host_ptr[0]) & 0xffffffff);
			desc.data_buff_1_MSB = ((uint64_t)(cmd->host_ptr[0]) >> 32);	
		} else {
			desc.data_buff_1_LSB = ((uint64_t)(nand_DmaCtrl[tab_id].DescSt[head_idx].phy_MPge) & 0xffffffff);
			desc.data_buff_1_MSB = ((uint64_t)(nand_DmaCtrl[tab_id].DescSt[head_idx].phy_MPge) >> 32);
		}
	}

	if (cmd->data_len[1]) {
		if (cmd->host_ptr[1]) {
			desc.data_buff_2_LSB = ((uint64_t)(cmd->host_ptr[1]) & 0xffffffff);
			desc.data_buff_2_MSB = ((uint64_t)(cmd->host_ptr[1]) >> 32);	
		} else {
			desc.data_buff_2_LSB = ((uint64_t)(nand_DmaCtrl[tab_id].DescSt[head_idx].phy_MPge + KERNEL_PAGE_SIZE) & 0xffffffff);
			desc.data_buff_2_MSB = ((uint64_t)(nand_DmaCtrl[tab_id].DescSt[head_idx].phy_MPge + KERNEL_PAGE_SIZE) >> 32);
		}
	}

	if (cmd->data_len[2]) {
		if (cmd->host_ptr[2]) {
			desc.data_buff_3_LSB = ((uint64_t)(cmd->host_ptr[2]) & 0xffffffff);
			desc.data_buff_3_MSB = ((uint64_t)(cmd->host_ptr[2]) >> 32);	
		} else {
			desc.data_buff_3_LSB = ((uint64_t)(nand_DmaCtrl[tab_id].DescSt[head_idx].phy_MPge + (KERNEL_PAGE_SIZE << 1)) & 0xffffffff);
			desc.data_buff_3_MSB = ((uint64_t)(nand_DmaCtrl[tab_id].DescSt[head_idx].phy_MPge + (KERNEL_PAGE_SIZE << 1)) >> 32);
		}
	}

	if (cmd->data_len[3]) {
		if (cmd->host_ptr[3]) {
			desc.data_buff_4_LSB = ((uint64_t)(cmd->host_ptr[3]) & 0xffffffff);
			desc.data_buff_4_MSB = ((uint64_t)(cmd->host_ptr[3]) >> 32);	
		} else {
			desc.data_buff_4_LSB = ((uint64_t)(nand_DmaCtrl[tab_id].DescSt[head_idx].phy_MPge + (KERNEL_PAGE_SIZE * 3)) & 0xffffffff);
			desc.data_buff_4_MSB = ((uint64_t)(nand_DmaCtrl[tab_id].DescSt[head_idx].phy_MPge + (KERNEL_PAGE_SIZE * 3)) >> 32);
		}
	}

	if (cmd->oob_len) {
		if (((req_flag & DF_BIT_DATA_IO) && (req_flag & DF_BIT_WRITE_OP)) \
		    || !(cmd->oob_ptr)) {
			if (cmd->oob_ptr) {
				memcpy (nand_DmaCtrl[tab_id].DescSt[head_idx].virt_oob, cmd->oob_ptr, \
					cmd->oob_len);
			}
			desc.oob_data_LSB = ((uint64_t)(nand_DmaCtrl[tab_id].DescSt[head_idx].phy_oob) \
					     & 0xffffffff);
			desc.oob_data_MSB = ((uint64_t)(nand_DmaCtrl[tab_id].DescSt[head_idx].phy_oob) \
					     >> 32);
		} else {
			desc.oob_data_LSB = ((uint64_t)(cmd->oob_ptr) & 0xffffffff);
			desc.oob_data_MSB = ((uint64_t)(cmd->oob_ptr) >> 32);
		}
	}

	if (req_flag & DF_BIT_READ_OP || req_flag & DF_BIT_STATUS_OP) {
		desc.dir = 0x1;
		desc.chk_Rdy_Bsy = 0x1;
		if (req_flag & DF_BIT_STATUS_OP) {
			desc.command = CMD_READ_STATUS_ENH;
			desc.no_ecc = DISABLE_ECC;
		} else if (req_flag & DF_BIT_CONFIG_IO) {
			desc.no_ecc = DISABLE_ECC;
			desc.command = CMD_READ_PARAMETER;
		} else {
			desc.command = CMD_READ;
		}
	} else if (req_flag & DF_BIT_ERASE_OP) {
		desc.chk_Rdy_Bsy = 0x1;
		desc.no_ecc = DISABLE_ECC; 
		desc.command = CMD_BLOCK_ERASE;
	} else if (req_flag & DF_BIT_WRITE_OP) {
		if(g_NvmeCtrl.pex_count ==1){
			if ((!lun) && (!target)&& (!(cmd->phyaddr.block_no & 1))) {
				desc.chk_Rdy_Bsy = 0x1;
			}
		} else {
			if ((!lun) && (!target)){
				desc.chk_Rdy_Bsy = 0x1;
			}
		}
		desc.command = CMD_PAGE_PROG;
	} else if (req_flag & DF_BIT_RESET_OP) {
		desc.no_ecc = DISABLE_ECC;
		desc.no_data = SET_NO_DATA;
		desc.command = CMD_RESET;
	}else if (req_flag & DF_BIT_SETFEAT_OP) {
		memset(nand_DmaCtrl[tab_id].DescSt[head_idx].virt_MPge,0x0, 0x1000);
		memcpy(nand_DmaCtrl[tab_id].DescSt[head_idx].virt_MPge,cmd->host_ptr[0],cmd->data_len[0]);
		desc.data_buff_1_LSB = ((uint64_t)(nand_DmaCtrl[tab_id].DescSt[head_idx].phy_MPge) & 0xffffffff);
		desc.data_buff_1_MSB = ((uint64_t)(nand_DmaCtrl[tab_id].DescSt[head_idx].phy_MPge) >> 32);
		desc.row_addr = cmd->data_len[1];
		desc.data_buff_2_LSB = 0;
		desc.data_buff_2_MSB = 0;
		cmd->data_len[1] = 0;
		desc.chk_Rdy_Bsy = 0x1;
		desc.no_ecc = DISABLE_ECC;
		desc.command = CMD_SET_FEATURE;
	}

#ifdef NAND_MSI 
	desc.irq_en = 1;
#else
	desc.irq_en = 0;
#endif
	desc.hold = 0;
	desc.desc_id = head_idx+1;
	desc.OwnedByfpga = 1;
	desc.is_Mul_pln = cmd->is_Mul_Pln;

	nand_DmaCtrl[tab_id].DescSt[head_idx].req_flag = req_flag;
	nand_DmaCtrl[tab_id].DescSt[head_idx].req_ptr = cmd->req_info;
	nand_DmaCtrl[tab_id].DescSt[head_idx].valid = 1;
	nand_DmaCtrl[tab_id].DescSt[head_idx].chip = tab_id;
	nand_DmaCtrl[tab_id].DescSt[head_idx].index = head_idx;
#if 0
	for(loop=0; loop<8; loop++) {
		memcpy((uint32_t *)(nand_DmaCtrl[tab_id].DescSt[head_idx].ptr)+loop, \
		       (uint32_t*)&desc+loop, sizeof(uint32_t)); 
	}
#endif
	memcpy(nand_DmaCtrl[tab_id].DescSt[head_idx].ptr,	\
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

uint32_t df_setup_nand_desc (cmd_info_t *cmd) 
{
	uint8_t i;
	uint32_t ret = 0;
	ret = df_fill_dma_desc (cmd);

#ifdef GET_CMD_STATUS
	if ((!(cmd->req_flag & DF_BIT_READ_OP)) && (cmd->req_flag & DF_BIT_CMD_END)) {
		for (i=0; i<4; i++) {
			cmd->host_ptr[i] = NULL;
			cmd->data_len[i] = 0;
		}
		cmd->data_len[0] = 1;
		cmd->oob_ptr = NULL;
		cmd->oob_len = 0;
		cmd->req_flag = cmd->req_flag | DF_BIT_STATUS_OP;
		ret |= df_fill_dma_desc (cmd);
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

	if((fp = fopen("error_log.txt","a"))==NULL) {
		printf("Error opening the file\n");
	}

	for(tab_no=0; tab_no<NAND_TABLE_COUNT; tab_no++){
		tab_ptr = n->fpga.nand_dma_regs.table[tab_no];
		for(desc_no= 0; desc_no< NAND_DESC_PER_TABLE; desc_no++,i++) {
			nand_DmaCtrl[tab_no].DescSt[desc_no].ptr = (uint8_t *)tab_ptr + (desc_no * NAND_DESC_SIZE);
			nand_DmaCtrl[tab_no].DescSt[desc_no].req_flag = 0;
			nand_DmaCtrl[tab_no].DescSt[desc_no].virt_oob = ls2_virt_addr[0] + 0x300000 + (i * ACTL_OOB_SIZE);
			nand_DmaCtrl[tab_no].DescSt[desc_no].phy_oob = (uint8_t*)(gc_phy_addr[0] + 0x300000 + (i * ACTL_OOB_SIZE));
			nand_DmaCtrl[tab_no].DescSt[desc_no].virt_MPge = ls2_virt_addr[0] + (i * ACTL_PAGE_SIZE);
			nand_DmaCtrl[tab_no].DescSt[desc_no].phy_MPge = (uint8_t*)(gc_phy_addr[0] + (i * ACTL_PAGE_SIZE));

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
	int ret;

	for (loop=0; loop<NAND_TABLE_COUNT; loop++) {
		*(regs->table_sz[loop]) =  NAND_DESC_PER_TABLE_DUP;/*toremove*/
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
	
	ret = pthread_create (&g_NvmeCtrl.io_completer_tid_nand, NULL, df_io_completer, (void *)&g_NvmeCtrl);
	if(ret) perror("io_completer for NAND");
	
	ret = pthread_create (&g_NvmeCtrl.unalign_bio_tid, NULL, unalign_bio_process, (void *)&g_NvmeCtrl);
	if(ret) perror("Unaligned Bio Handler");
	
}

void deinit_df_dma_mgr (void)
{
	int i;
	for (i=0; i<5; i++) {
		munmap(ls2_virt_addr[i] , (1024*getpagesize()));
	}
	THREAD_CANCEL(g_NvmeCtrl.io_completer_tid_nand);
	THREAD_CANCEL(g_NvmeCtrl.unalign_bio_tid);
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

	set_thread_affinity(4, pthread_self());
#ifdef NAND_MSI 
	int seln, fd, count, nb;
	fd_set readfds;
	static struct timeval def_time = {0, 1000};
	static struct timeval timeout = {0, 1000};
	uint32_t info = 1;

	fd = open ("/dev/uio1", O_RDWR);
	if (fd < 0) {
		perror("open uio1");
	}
	FD_ZERO (&readfds);

	while (1) {
WAIT_INT:
		timeout = def_time;
		FD_SET (fd, &readfds);
		info = 1;
		nb = write(fd, &info, sizeof(info));
		*(n->fpga.nand_gpio_int) = 0x0;

		seln = select (fd + 1, &readfds, NULL, NULL, &timeout);
		if (seln > 0 && FD_ISSET (fd, &readfds)) {
			read (fd, &count, sizeof (uint32_t));
		}
#endif
		while (1) {
			do {
				if (Desc_trck[desc_tail_idx].is_used == TRACK_IDX_USED) {
					break;
				}
#ifdef NAND_MSI 
				goto WAIT_INT;
#else
				usleep(1);
#endif
			} while (1);

			do {
				mutex_lock (&(Desc_trck[desc_tail_idx].DescSt_ptr->available));
				if (Desc_trck[desc_tail_idx].DescSt_ptr->valid) {
					break;
				}
				mutex_unlock (&(Desc_trck[desc_tail_idx].DescSt_ptr->available));
				//usleep(1);
			}  while (1);

			do {
				csf = (nand_csf *)((uint32_t*)(Desc_trck[desc_tail_idx].DescSt_ptr->ptr) + 15);
				csf_val = *((uint32_t *)csf);
				csf = (nand_csf *)&csf_val;
				//usleep(1);
			}while (csf->OwnedByFpga);

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
						if (((*(Desc_trck[desc_tail_idx].DescSt_ptr->virt_MPge) & 0x42) == 0x42) && \
						    ((*(Desc_trck[desc_tail_idx].DescSt_ptr->virt_MPge) & 0x21) == 0x21)) {
							io_stat = 1;
							printf("ERROR: write operation fails with status : %x @ %lx desc\n", \
							       *(Desc_trck[desc_tail_idx].DescSt_ptr->virt_MPge),desc_tail_idx);
							printf("chip:%u, index:%u\n",Desc_trck[desc_tail_idx].DescSt_ptr->index, \
							       Desc_trck[desc_tail_idx].DescSt_ptr->chip);
							while(1);
						}
					} else if (flag & DF_BIT_ERASE_OP) {
						if ((*(Desc_trck[desc_tail_idx].DescSt_ptr->virt_MPge) & 0x21) == 0x21) {
							io_stat = 1;
							printf("ERROR: erase operation fails with status : %x\n", \
							       *(Desc_trck[desc_tail_idx].DescSt_ptr->virt_MPge));
						}
					}
					status |= io_stat;
				}
				if (flag & DF_BIT_CMD_END) {
					if ((flag & DF_BIT_DATA_IO)	|| (flag & DF_BIT_GC_OP)) {
						dm_df_completion_cb (Desc_trck[desc_tail_idx].DescSt_ptr->req_ptr,status);
						status = 0;
						io_stat = 0;
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
			}
#else
#if 0
			if ((flag & DF_BIT_DATA_IO)	|| (flag & DF_BIT_GC_OP)) {
				if ((flag & DF_BIT_IS_OOB) || (flag & DF_BIT_ERASE_OP) || \
				    ((flag & DF_BITS_DATA_PAGE) && (flag & DF_BIT_READ_OP))) {
					dm_df_completion_cb (Desc_trck[desc_tail_idx].DescSt_ptr->req_ptr,io_stat);
				}
			} else if (flag & DF_BIT_MGMT_IO || flag & DF_BIT_SEM_USED) {
				sem_post((sem_t*)Desc_trck[desc_tail_idx].DescSt_ptr->req_ptr);
			}
#endif
			if (flag & DF_BIT_CMD_END) {
				if ((flag & DF_BIT_DATA_IO) || (flag & DF_BIT_GC_OP)) {
					dm_df_completion_cb (Desc_trck[desc_tail_idx].DescSt_ptr->req_ptr,status);
					status = 0;
					io_stat = 0;
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
#ifdef NAND_MSI
		}
#endif
	}
}
