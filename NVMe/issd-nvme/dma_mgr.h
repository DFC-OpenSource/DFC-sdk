#ifndef __DMA_MGR_H
#define __DMA_MGR_H

#include <stdint.h>
#include <semaphore.h>

#include "dma_ddr.h"
#include "dma_nand.h"

#define DDR_MEM_TYPE  0
#define NAND_MEM_TYPE 1

#define FPGA_DDR_NS 2
#define FPGA_NAND_NS 4
#define LS2_DDR_NS 8

#define DDR_REQTYPE_WRITE 0
#define DDR_REQTYPE_READ  1

#define DESC_TBL_COUNT          1 	 	/*Total number of Tables(each table represent one ddr)*/
#define DESC_TBL_ENTRIES        128 		/*Maximum descriptor count per table*/

//#define MAX_LBA_COUNT 		8388608         /* 8GB * 4 CARDs Maximum number of Logical block address - meant for DDR-FTL -TODO*/
#define MAX_LBA_COUNT 		1048576         /* single 8GB Maximum number of Logical block address - meant for DDR-FTL -TODO*/
#define DDR_PAGE_SIZE 		4096            /*Page size*/

//#define DESC_TBL_OFFSET     0X00000000000010000 /*PCIe register base address of descriptor table */
#define DESC_TBL_OFFSET     0X00000000000040000 /*PCIe register base address of descriptor table */
//#define CSR_OFFSET          0x0000000000000000
#define CSR_OFFSET          0x0000000000002000

#define INTR_OFFSET          0X0000000000000001 /*Interrupt control register base address*/
#define DESC_SIZE_OFFSET     0X0000000000000002 /*Descriptor Table Size Register base address*/
#define DESC_SIZE            32 /*0x20*/


/**
 * @Structure 	: desc_info
 * @Brief       : This structure will act as the index of the entire descriptors.
 * @Members     :
 *  @ptr 	 : Pointer to the fpga memory where the particular descriptor is stored.
 *  @available 	 : Flag to indicate wheather the particular descriptor can be used for dma operation.
 *                 nvme bio thread sees if set to 1, it resets to 0 and uses the available desc;
 *                 completion thread sees if set to 0,( and if for_ftl also 0 in NAND case), uses desc status 
 *                 for completion info and sets status as 1 to make it available
 *  if (MEMORY_TYPE == NAND_MEM_TYPE)
 *  @for_ftl   	 : Flag to indicate wheather the particular descriptor is used for internal FTL operations.
 *                 completion thread sees if set to 1, it drops processing this desc; else checks available flag
 *                 nvme bio thread sets to 1 for internal FTL ops (and ready reset to 0) and when dma's done, it can reset
 *                 for_ftl to 0 and ready to 1
 */
#define LOCK_WITH_MUTEX 1
#define SEMAPHORE_LOCK
typedef struct DescStatus {
	uint64_t         *ptr;
	volatile void *req_ptr;
#ifdef SEMAPHORE_LOCK
	sem_t free_desc;
	sem_t used_desc;
#else
#if (LOCK_WITH_MUTEX==0)
	volatile uint8_t available;
#else
	pthread_mutex_t  available;
#endif
	volatile uint8_t valid;
#endif
#if (MEMORY_TYPE == NAND_MEM_TYPE)
	volatile uint8_t for_ftl;
#endif
} DescStatus;

typedef struct csr_reg {
	uint32_t    cmd_tag:11;
	uint32_t    err_code:3;
#define NO_IRQ  0
#define WR_TOUT 1
#define RD_TOUT 2
#define USR_IRQ 3
#define HOLD_IRQ 4
	uint32_t    start:1;
	uint32_t    loop:1;
	uint32_t    reset:1;
	uint32_t    rsvd:15;
} csr_reg;

typedef struct DmaRegs {
	uint32_t     *csr[2];
	uint32_t     *icr[2];
	uint32_t     *table_sz[2];
	uint64_t     *table[2];
} DmaRegs;

/** 
 * @Structure : DmaCtrl
 * @Brief     : Consists all dma usage, control and grouping info
 * @Members   :
 *  @regs       : dma register addresses to control dma machine inside fpga
 *  @desc_st    : status info of each and every descriptor
 *  @mem_type   : to identify which descritor structure for ddr|nand
 *  @head_idx   : index of latest descriptor used from organized desc array
 *  @tail_idx   : index of latest descriptor freed in organized desc array
 *  @total_desc : Total number of decriptors
 *  @is_active  : used by dma init/deinit calls to safely operate on desc list
 *  			  and ensure proper exit
 */
typedef struct DmaCtrl {
	DmaRegs      *regs;
	DescStatus   *desc_st;
	uint8_t      mem_type;
	uint16_t     head_idx;
	uint16_t     tail_idx;
	uint16_t     total_desc;
	volatile uint32_t is_active;
	uint8_t      io_processor_st;
#define IOPROC_ST_WAIT_DESC1  10
#define IOPROC_ST_GOT_DESC1   20
#define IOPROC_ST_REL_DESC1   30
#define IOPROC_ST_WAIT_DESC2  40
#define IOPROC_ST_GOT_DESC2   50
#define IOPROC_ST_REL_DESC2   60
#define IOPROC_ST_ON_OCM      70
#define IOPROC_ST_DONE_OCM    80
#define IOPROC_ST_END   90
	uint8_t      io_completer_st;
#define IOCOMP_ST_WAIT_VALID  10
#define IOCOMP_ST_WAIT_DESC3  20
#define IOCOMP_ST_GOT_DESC3   30
#define IOCOMP_ST_REL_DESC3   40
#define IOCOMP_ST_REL_INV_DESC3		41
#define IOCOMP_ST_REL_SKIP_DESC3	42
#define IOCOMP_ST_REL_END_DESC3		43
#define IOCOMP_ST_ON_OCM     50
#define IOCOMP_ST_DONE_OCM   60
#define IOCOMP_ST_WAIT_OWN   70
#define IOCOMP_ST_WAIT_SKP   80
} DmaCtrl;

/**
 * @Func	: dma_default
 * @Brief	: do initialisation for structures which will be used while making the dma descriptor.
 *  @input	: none.
 *  @return 	: return zero on success and non zero on init failure.
 Init done properly in fpga can be known here or wait for init completion signal from  fpga or some  regsiter?
 */
void dma_default (void);

/**
 * @Func	: exit_dma
 * @Brief	: will be used to free the memory allocated dynamically(if any) for dma descriptor creation.
 *  @input      : none.
 *  @return 	: none.
 */
void exit_dma (void);


/**
 * @Func	: reset_dma
 * @Brief	: reset the DMA related structures and parameters.
 *  @input      : none.
 *  @return 	: Zero on success and non zero on failure.
 */
int reset_dma();

/**
 * @Func	: make_dma_desc
 * @Brief	: Function which makes the DMA descriptor for DDR interfaced with FPGA 
 *		  and map them on the PCIe BAR registers.
 *  @input	: nvme_bio - Contains the parameters used for I/O operation between x86 
 *		  and storage DDR.
 *  @return 	: Return zero on success and non zero on failure.
 */
int make_dma_desc (uint64_t prp, uint64_t pa, uint64_t len, void *req_info, uint8_t req_type, uint8_t eoc);
void inline deinit_dma_mgr (void);
void *io_completer (void *arg);

#endif /*#ifdef __DMA_MGR_H*/
