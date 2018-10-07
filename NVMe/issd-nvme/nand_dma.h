
#include <stdint.h>
#include <pthread.h>

#define NAND_CSR_OFFSET		0x200
#define NAND_TABLE_OFFSET	0x200
#define NAND_TBL_SZ_SHIFT	0x1
#define NAND_TABLE_COUNT	8
#define NAND_DESC_PER_TABLE 	128
#define NAND_DESC_SIZE		2
#define TOTAL_NAND_DESCRIPTORS (NAND_TABLE_COUNT*NAND_DESC_PER_TABLE)

#define ECC_EN				0
#define ECC_DIS				1
#define NAND_ERASE 			0x05
#define NAND_PROG_PAGE 		0x0A
#define NAND_COL_PROG_PAGE	0x0C
#define NAND_READ_STS		0x14
#define NAND_READ_STS_EN	0x15
#define NAND_READ 			0x1E

#define CMD_STRUCT_SIZE 	48

#define NAND_SEMAPHORE_LOCK

#define DESCST_PTR
enum BDBM_DESC_TYPE {
	FTL_IO = 0,
	FTL_RD_OOB,
	FTL_WR_OOB,
	FTL_PMT,
	FTL_ERASE,
#ifdef CMD_STATUS_DESC
	NAND_CMD_STS,
	NAND_RD_STS,
#endif
	DM_MAGIC,
	DM_BAD_BLK,
	DM_FISRT_PAGE,
	DM_ALL_PAGES,
	NAND_RESET,
};

typedef struct nand_cmd_struct {
	unsigned int row_addr;
	unsigned int column_addr;
	unsigned int buffer_channel_id;
	unsigned int len_target_sel;
	unsigned int direction_control;
	unsigned int reserv1;
	unsigned int control;
	unsigned int reserv2;
	unsigned int command_sel;
	unsigned int data_buffer_LSB;
	unsigned int data_buffer_MSB;
	unsigned int reserv3;
}nand_cmd_struct;

typedef struct Nand_DescStatus {
	uint8_t         *ptr;
	uint8_t         req_type;
	void            *req_ptr;
	uint8_t         *phy_oob;
	uint8_t         *virt_oob;
	uint64_t 		cmdbuf_phy_addr;
	uint64_t		cmdbuf_virt_addr;
#ifdef PMT_ON_FLASH
	uint8_t         *phy_page;
	uint8_t         *virt_page;
#endif
#ifdef NAND_SEMAPHORE_LOCK
	sem_t free_desc;
	sem_t used_desc;
#else
	pthread_mutex_t available;
	volatile uint8_t valid;
#endif
} DescStat;

typedef struct nand_descriptor {
	uint64_t cmd_struct_addr:40;
	uint8_t  rsvd;
	uint8_t  irq_en:1;
	uint8_t  hold:1;
	uint8_t  dma_cmp:1;
	uint16_t desc_id:12;
	uint8_t  OwnedByfpga:1;
} nand_descriptor;

typedef struct nand_csf_reg {
	uint8_t     cmd_struct_addr;
	uint8_t     rsvd;
	uint8_t     irq_en:1;
	uint8_t     hold:1;
	uint8_t     dma_cmp:1;
	uint16_t    desc_id:12;
	uint8_t     OwnedByFpga:1;
} nand_csf;

typedef struct nand_csr_reg {
	uint8_t     desc_id;
	uint8_t     err_code:3;
	uint8_t     start:1;
	uint8_t     loop:1;
	uint8_t     reset:1;
	uint32_t    rsvd:18;
} nand_csr_reg;

typedef struct Nand_DmaRegs {
	uint32_t 	 *csr[NAND_TABLE_COUNT];
	uint32_t     *table_sz[NAND_TABLE_COUNT];
	uint32_t     *table[NAND_TABLE_COUNT];
} Nand_DmaRegs;

typedef struct DmaControl {
	DescStat    DescSt[NAND_DESC_PER_TABLE];
	uint16_t    head_idx;
	uint16_t    tail_idx;
}Nand_DmaCtrl;
#define FREED	0
#define	FILLED	1
typedef struct Desc_Track {
#ifdef DESCST_PTR
	DescStat    *DescSt_ptr;
	uint8_t	    is_used;
#else
	uint8_t     tid;
	uint32_t    idx;
#endif
} Desc_Track;

void nand_dma_default (Nand_DmaRegs *regs);
uint8_t make_desc(uint8_t tid,void *req_ptr,uint8_t req_type,nand_cmd_struct *cmd_struct);
