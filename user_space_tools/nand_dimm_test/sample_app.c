#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include "nand_dma.h"

atomic_t wait_to_complete = ATOMIC_INIT(0);
extern pthread_t io_completion;
struct timeval start,end;
extern fpga_version;
extern int CHIP_COUNT;

void sig_handler(int signo)
{
	if (signo == SIGINT) {
		printf("received SIGINT\n");
		exit(0);
	}
}

int setup_ctrl_c_handler (void)
{
	if (signal(SIGINT, sig_handler) == SIG_ERR) {
		perror ("signal():");
	}
	return 0;
}

int nand_callback(io_cmd *cmd)
{
	if (cmd->status) {
		//		printf("Cmd_status -> SUCCESS\n");
	}else {
		//		printf("CMD_status -> FAILURE\n");
	}

	free(cmd);
	atomic_dec(&wait_to_complete);
	return 0;
}

/* To achieve More throughput we are doing Multi-LUN and Multi-target Operations.
 * Using two LUNs and two targets is recommended*/

/* This example program does read|write from page-0 of each block, 
 * user can modify the code to do read|write for all 512 pages of block*/
int main(int argc,char **argv)
{
	uint8_t *va; int opt=0;
	uint8_t lun, target,chip,ret,chip_no;
	uint16_t block, page, length,blk_num =0;
	int16_t blk_cnt;
	float time=0,size = 0,bw=0;

	if(argc != 4 && argc !=3) {
		printf("Usage: ./<exe_name> <1-PAGE_PROG | 2-PAGE_READ> <chip>  <Block_count>\n");
		return FAILURE;
	}

	opt = atoi(argv[1]);
	chip_no = atoi(argv[2]);
	if(chip_no < 0 && chip_no > 7) {
		printf("Wrong Chip\n");
		return FAILURE;
	}
	if(argc == 4){
		blk_cnt = atoi(argv[3]);
		if(blk_cnt & 0x1) {blk_cnt++;}   /*Block cnt in even number to access two planes*/
		if(blk_cnt > NAND_BLOCK_COUNT) {blk_cnt = NAND_BLOCK_COUNT;}
		if(blk_cnt < 2) {blk_cnt = 2;}
	}else{
		blk_cnt = 2;
	}

	setup_ctrl_c_handler();
	ret = nand_dm_init();
	if(ret) {
		perror("nand_dm_init:");
		return FAILURE;
	}

#if 0
	if(fpga_version != 0x00030301) {
		printf("NAND LIBRARY won't support this FPGA image : %x, Supported Version : 03.00.04\n",fpga_version);
		return FAILURE;
	}
#endif
	io_cmd *cmd;

	/*Erasing blocks from LUNs-2, Targets-2, Chips-8 before writing to the block*/
	switch(opt){
		case 1:
			for(block = 0; block < blk_cnt; block++){
				for(lun = 0; lun< NAND_LUN_COUNT; lun++){
					for( target = 0; target < NAND_TARGET_COUNT; target++){
						/*for (chip = 0; chip < CHIP_COUNT; chip++) {*/
							cmd = malloc(sizeof(io_cmd));
							memset(cmd , 0 , sizeof(io_cmd));
							cmd->chip = chip_no;
							cmd->target = target;
							cmd->lun = lun;
							cmd->block = block;
							cmd->host_addr[0] = 0;
							cmd->len[0] = 0;
							atomic_inc(&wait_to_complete);
							nand_block_erase(cmd);              /*Library Call to erase a block*/
							/*}*/
					}
				}
			}
			/*goto DEALLOC;*/
			/* Have allocated 4MB * 70 units of memory in uio_pci_generic driver.
			 * physical address and respective virtual address for each 4MB physical memory units(70 units) 
			 * are saved in the variables virt_addr and phy_addr, first virtual
			 * memory virt_addr[0] is used by the library, other 69-4MB units can be used by the
			 * userspace application.*/

			/*Page Write- 16K + 1K OOB*/
			lun = target =0;
			va = virt_addr[1];
			memset(va, 0xab, 0x4000);
			//memset((va+0x4000), 0xcd, 0x400); /*For OOB*/

			/* To get Better throughput we are doing Multi-Plane operations in write.
			 * Always write consecutive blocks for Multi-plane Operation.*/

			gettimeofday(&start, NULL);
			for(block = 0; block < blk_cnt; block += 2){
				for(page = 0; page < NAND_PAGES_PER_BLOCK ; page++) {
					for(lun = 0;lun < NAND_LUN_COUNT; lun++){
						for(target = 0;target < NAND_TARGET_COUNT; target++){
							/*for(chip = 0; chip < CHIP_COUNT; chip++){*/
								cmd = malloc(sizeof(io_cmd));
								memset(cmd, 0, sizeof(io_cmd));

								cmd->lun = lun;
								cmd->target = target;
								cmd->block = block;
								cmd->chip = chip_no;
								cmd->len[0] = 0x1000; cmd->len[1] = 0x1000;
								cmd->len[2] = 0x1000; cmd->len[3] = 0x1000;
								cmd->len[4] = 0x0;
								//cmd->len[4] = 0x400;    /*OOB length 1KB*/
								/*phy_addr[1] = 0x15e4f00000;*/
								cmd->page = page;
								cmd->host_addr[0] = (uint64_t) phy_addr[1];
								cmd->host_addr[1] = (uint64_t) ((uint8_t *)phy_addr[1]+0x1000);
								cmd->host_addr[2] = (uint64_t) ((uint8_t *)phy_addr[1]+0x2000);
								cmd->host_addr[3] = (uint64_t) ((uint8_t *)phy_addr[1]+0x3000);
								cmd->host_addr[4] = 0;
								//cmd->host_addr[4] = (uint64_t) ((uint8_t *)phy_addr[1]+0x4000); /*OOB's source data_buffer*/

								atomic_inc(&wait_to_complete);
								nand_page_prog(cmd);

								cmd = malloc(sizeof(io_cmd));
								memset(cmd, 0, sizeof(io_cmd));
								cmd->lun = lun;
								cmd->target = target;
								cmd->block = block+1;
								cmd->chip = chip_no;
								cmd->len[0] = 0x1000; cmd->len[1] = 0x1000;
								cmd->len[2] = 0x1000; cmd->len[3] = 0x1000;
								cmd->len[4] = 0;
								//cmd->len[4] = 0x400;    /*OOB length 1KB*/

								cmd->page = page;
								cmd->host_addr[0] = (uint64_t) phy_addr[1];
								cmd->host_addr[1] = (uint64_t) ((uint8_t *)phy_addr[1]+0x1000);
								cmd->host_addr[2] = (uint64_t) ((uint8_t *)phy_addr[1]+0x2000);
								cmd->host_addr[3] = (uint64_t) ((uint8_t *)phy_addr[1]+0x3000);
								cmd->host_addr[4] = 0;
								//cmd->host_addr[4] = (uint64_t) ((uint8_t *)phy_addr[1]+0x4000);

								atomic_inc(&wait_to_complete);
								nand_page_prog(cmd);
								/*}*/
						}
					}
				}
			}
			break;
		case 2:
			/*Page Read -16K + 1K OOB*/
			gettimeofday(&start, NULL);
			for(block = 0; block < blk_cnt; block += 2){
				for(page = 0; page < NAND_PAGES_PER_BLOCK ; page++){
					for(lun = 0; lun < NAND_LUN_COUNT; lun++){
						for(target = 0; target < NAND_TARGET_COUNT; target++){
							/*for(chip = 0; chip < CHIP_COUNT; chip++){*/
								cmd = malloc(sizeof(io_cmd));
								memset(cmd, 0, sizeof(io_cmd));
								memset(virt_addr[1], 0, 0x8000);
								cmd->lun = lun;
								cmd->target = target;
								cmd->block = block;
								cmd->chip = chip_no;
								cmd->len[0] = 0x1000; cmd->len[1] = 0x1000; 
								cmd->len[2] = 0x1000; cmd->len[3] = 0x1000;
								cmd->len[4] = 0x0;
								//cmd->len[4] = 0x400;    /*OOB length 1KB*/
								/*phy_addr[2] = 0x15e4f10000;*/
								cmd->col_addr = 0x0;
								cmd->page = page;
								cmd->host_addr[0] = (uint64_t) phy_addr[1];
								cmd->host_addr[1] = (uint64_t) (phy_addr[1]+0x1000);
								cmd->host_addr[2] = (uint64_t) (phy_addr[1]+0x2000);
								cmd->host_addr[3] = (uint64_t) (phy_addr[1]+0x3000);
								cmd->host_addr[4] = 0;
								//cmd->host_addr[4] = (uint64_t) (phy_addr[2]+0x4000); /*data_buffer to hold OOB data*/

								atomic_inc(&wait_to_complete);
								nand_page_read(cmd);                    /*Library call to page read*/

								cmd = malloc(sizeof(io_cmd));
								memset(cmd, 0, sizeof(io_cmd));
								cmd->lun = lun;
								cmd->target = target;
								cmd->chip = chip_no;
								cmd->block = block+1;
								cmd->len[0] = 0x1000; cmd->len[1] = 0x1000;
								cmd->len[2] = 0x1000; cmd->len[3] = 0x1000;
								cmd->len[4] = 0x0;
								//cmd->len[4] = 0x400;    /*OOB length 1KB*/

								cmd->col_addr = 0x0;
								cmd->page = page;
								cmd->host_addr[0] = (uint64_t) (phy_addr[1]+0x5000);
								cmd->host_addr[1] = (uint64_t) (phy_addr[1]+0x6000);
								cmd->host_addr[2] = (uint64_t) (phy_addr[1]+0x7000);
								cmd->host_addr[3] = (uint64_t) (phy_addr[1]+0x8000);
								cmd->host_addr[4] = 0;
								//cmd->host_addr[4] = (uint64_t) (phy_addr[2]+0x9000); /*data_buffer to hold OOB data*/

								atomic_inc(&wait_to_complete);
								nand_page_read(cmd);
								/*}*/
						}
					}
				}
			}
			break;
		default:
			printf("Wrong Option\n");
			return 0;
	}
	/*Can use either atomic operations or END_DESC to ensure DMA Completion*/
#if 1 
	make_desc(NULL, 0, END_DESC, NULL);
	pthread_join(io_completion, NULL);
#else
	while(atomic_read(&wait_to_complete)) {
		usleep(1);
	}
#endif
	gettimeofday(&end, NULL);
 	/*(LUN_cnt * TRGT_cnt * CHIPS_cnt * 16KB_per_trsfer + 1KB_OOB * block_cnt)*/
	//size = (float)( 2 * 2 * 8 * 17 * blk_cnt);
	/*(LUN_cnt * TRGT_cnt * CHIPS_cnt * 16KB_per_trsfer * pager_per_block *block_count)*/
	size = ((float)(2 * 2 * 16 * NAND_PAGES_PER_BLOCK * blk_cnt))/1024;
	time = ((float)((1000000 * (end.tv_sec - start.tv_sec)) + (end.tv_usec - start.tv_usec)))/1000000;
	bw = size/time;
	printf("Size: %fMB Time: %fsec \nBandwidth: %fMB/s\n",size,time,bw);
	
	nand_dm_deinit();
	return 0;
}
