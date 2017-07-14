#ifndef __DMA_DF_NAND_H
#define __DMA_DF_NAND_H

#include <stdint.h>
#include "bdbm_ftl/bdbm_drv.h"

#ifndef BVAL
#define BVAL(n) (1 << n)
#endif

#define NAND_CSR_OFFSET			0x10
#define NAND_TABLE_OFFSET		0x200
#define NAND_TBL_SZ_SHIFT		0x1
#define NAND_TABLE_COUNT		8
#define NAND_DESC_PER_TABLE		16
#define NAND_DESC_PER_TABLE_DUP 32
#define NAND_DESC_SIZE			0x40
#define TOTAL_NAND_DESCRIPTORS 	(NAND_TABLE_COUNT*NAND_DESC_PER_TABLE)

#define CMD_STRUCT_SIZE 	48
enum nand_cmd_ctrl_bits {
	DISABLE_ECC                 = 1,
	SET_NO_DATA					= 1,
	SET_MUL_PLN_CMD				= 1,
};

enum nand_cmds_dma_mgr {
	CMD_RESERVED                = 0,
	CMD_RESET                   = 1,
	CMD_SRESET                  = 2,
	CMD_RESET_LUN               = 3,
	CMD_RESET_CHANNEL_ALL       = 4,
	CMD_BLOCK_ERASE             = 5,
	CMD_COPY_BACK_WRITE         = 8,
	CMD_SET_FEATURE             = 9,
	CMD_PAGE_PROG               = 10,
	CMD_PAGE_PROG_CACHE         = 11,
	CMD_CHANGE_WRITE_COL        = 12,
	CMD_GET_FEATURE             = 16,
	CMD_READ_UNIQUE             = 17,
	CMD_READ_PARAMETER          = 18,
	CMD_READ_ID                 = 19,
	CMD_READ_STATUS             = 20,
	CMD_READ_STATUS_ENH         = 21,
	CMD_CHANGE_READ_COL         = 24,
	CMD_CHANGE_READ_COL_ENH     = 25,
	CMD_COPY_BACK_READ          = 26,
	CMD_READ_CACHE_SEQ          = 27,
	CMD_READ_CACHE_RAND         = 28,
	CMD_READ_CACHE_END          = 29,
	CMD_READ                    = 30,
	CMD_LUN_SET_FEATURE         = 33,
	CMD_LUN_GET_FEATURE         = 34,
	CMD_BLK_ERASE_MULTIPLANE    = 35,
	CMD_PAGE_PROG_MULTIPLANE    = 36,
} nand_dma_mgr_cmds_t;

enum DESC_TRACK_STAT {
	TRACK_IDX_FREE = 0,
	TRACK_IDX_USED = 1,
};


typedef enum REQ_FLAGS {
	DF_BIT_READ_OP      = BVAL ( 1),
	DF_BIT_WRITE_OP     = BVAL ( 2),
	DF_BIT_ERASE_OP     = BVAL ( 3),
	DF_BIT_MGMT_IO      = BVAL ( 4),
	DF_BIT_DATA_IO      = BVAL ( 5),
	DF_BIT_IS_OOB       = BVAL ( 6),
	DF_BIT_IS_PAGE      = BVAL ( 7),
	DF_BIT_CMD_END		= BVAL ( 8),
	DF_BIT_GC_OP        = BVAL ( 9),
	DF_BIT_STATUS_OP    = BVAL (10),
	DF_BIT_SCAN_BB		= BVAL (11),
	DF_BIT_RESET_OP     = BVAL (12),
	DF_BIT_SEM_USED     = BVAL (13),
	DF_BIT_CONFIG_IO    = BVAL (14),
	DF_BIT_SETFEAT_OP   = BVAL (15),

	/* combos come now */
	DF_BITS_MGMT_OOB    = BVAL ( 4) | BVAL ( 6),
	DF_BITS_MGMT_PAGE   = BVAL ( 4) | BVAL ( 7),
	DF_BITS_DATA_OOB    = BVAL ( 5) | BVAL ( 6),
	DF_BITS_DATA_PAGE   = BVAL ( 5) | BVAL ( 7),
	/* might have more later.. */
}REQ_FLAGS_T;


#if 0
typedef struct nand_cmd_struct {
	uint32_t row_addr;
	uint32_t column_addr;
	uint32_t buffer_channel_id;
	uint32_t len_target_sel;
	uint32_t direction_control;
	uint32_t reserv1;
	uint32_t control;
	uint32_t reserv2;
	uint32_t command_sel;
	//uint64_t data_buffer;
	uint32_t data_buff_lsb;
	uint32_t data_buff_msb;
}nand_cmd_struct_t;
#endif
typedef struct nand_descriptor {
	uint32_t row_addr;
	uint32_t column_addr	:29;
	uint32_t target			:3;
	uint32_t data_buff_1_LSB;
	uint32_t data_buff_1_MSB;
	uint32_t data_buff_2_LSB;
	uint32_t data_buff_2_MSB;
	uint32_t data_buff_3_LSB;
	uint32_t data_buff_3_MSB;
	uint32_t data_buff_4_LSB;
	uint32_t data_buff_4_MSB;
	uint32_t data_len_1		:16;
	uint32_t data_len_2		:16;
	uint32_t data_len_3		:16;
	uint32_t data_len_4		:16;
	uint32_t oob_data_LSB;
	uint32_t oob_data_MSB	:20;
	uint32_t oob_data_len	:12;	
	uint32_t buffer_id		:16;
	uint32_t channel_id		:16;
	uint32_t dir			:1;
	uint32_t no_data		:1;
	uint32_t no_ecc			:1;
	uint32_t command		:6;
	uint32_t irq_en			:1;
	uint32_t hold			:1;
	uint32_t dma_cmp		:1;
	uint32_t desc_id		:8;
	uint32_t OwnedByfpga	:1;
	uint32_t chk_Rdy_Bsy	:1;
	uint32_t is_Mul_pln		:1;
	uint32_t rsvd1			:9;
} __attribute__((packed)) nand_descriptor_t;

typedef struct Nand_DescStatus {
	uint8_t         *ptr;
	uint32_t         req_flag;
	void            *req_ptr;
	uint8_t         *phy_oob;
	uint8_t         *virt_oob;
	uint8_t 		*phy_MPge;
	uint8_t 		*virt_MPge;
	uint64_t		cmdbuf_virt_addr;

	pthread_mutex_t available;
	volatile uint8_t valid;
	uint16_t		index;
	uint16_t		chip;
} DescStat;

typedef struct nand_csf_reg {
	uint8_t dir				:1;
	uint8_t no_data			:1;
	uint8_t no_ecc			:1;
	uint8_t cmd				:6;
	uint8_t irq_en			:1;
	uint8_t hold			:1;
	uint8_t dma_cmp			:1;
	uint16_t desc_id		:8;
	uint8_t OwnedByFpga		:1;
	uint32_t rsvd			:11;
} __attribute__((packed)) nand_csf;


typedef struct nand_csr_reg {
	uint8_t     desc_id;
	uint8_t     err_code	:3;
	uint8_t     start		:1;
	uint8_t     loop		:1;
	uint8_t     reset		:1;
	uint32_t    rsvd		:18;
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

typedef struct Desc_Track {
	DescStat    *DescSt_ptr;
	uint8_t	    is_used;
} Desc_Track;


#endif /*__DMA_DF_NAND_H*/
