
#ifndef __DDR_DMA_H
#define __DDR_DMA_H

#include <stdint.h>

#define COL_MASK        0x000003FF
#define BANK_MASK       0x00000007
#define ROW_MASK        0x0000FFFF
#define RANK_MASk       0x00000001

#define BANK_SHIFT      10
#define ROW_SHIFT       13
#define RANK_SHIFT      29
#define CHIP_SHIFT      30

#define DDR_RBC_ADDR(rbc,pba)                           \
	do {                                                \
		rbc.col  = pba & COL_MASK;                      \
		rbc.bank = (pba >> BANK_SHIFT) & BANK_MASK;     \
		rbc.row  = (pba >> ROW_SHIFT) & ROW_MASK;       \
		rbc.rank = (pba >> RANK_SHIFT) & RANK_MASK;     \
		rbc.chip = pba >> CHIP_SHIFT;                   \
	} while(0);

/*A DMA error should trigger ls2 to reset complete dma engine*/
#define CLEARALL_ON_ERR 1

/**
 * @Structure : rbc_addr
 * @Brief     : ddr3 will be addressed by filling row, column and bank fields separately.
 * @Members
 *  @col       : column number.
 *  @bank      : bank number.
 *  @row       : row number.
 *  @rank      : rank number.
 *  @chip      : DDR selection bit.
 *  @rsv1      : reserved bit.
 */
typedef struct rbc_addr {
	uint32_t col  : 10;
	uint32_t bank :  3;
	uint32_t row  : 16;
	uint32_t rank :  1;
	uint32_t chip :  1;
	uint32_t rsv1 :  1;
} rbc_addr;


/**
 * @Structure : phy_addr
 * @Brief     : 31 bit address. Incrementing with the page size moves to the next addr.
 * @Members
 *  @addr      : combined field of row,column and bank.
 */
typedef struct phy_addr{
	uint32_t addr : 31;
	uint32_t rsv1 :  1;
} phy_addr;

/*
#define DDR_ROW_ADDR(phy) ((rbc_addr)phy.row)
#define DDR_COL_ADDR(phy) ((rbc_addr)phy.col)
#define DDR_BANK_ADDR(phy) ((rbc_addr)phy.bank)
*/

/**
 * @Union       : ddr_addr_u
 * @Brief       : Contains two structure for addressing the ddr3.
 *                Based on the addressing way(in single field or addressing row,column and bank
 *                separately),any one of the structure will be used.
 * @Members     : 
 *  @rbc        : Will be used to address the ddr3 by filling the row,column and bank bits separately.
 *  @phy        : Will be used to address the ddr3 by increment 4KB space for every page request.
 */
typedef union ddr_addr_u {
	rbc_addr rbc;
	phy_addr phy;
} ddr_addr_u;

/**
 * @Structure	 : csf
 * @Brief	 : Control and status field.Extracts the last 64 byte of the descriptor structure for i/o completion thread.
 * @Members	 :
 *  @irq_en      : Interrupt bit.If this bit is set,then interrupt will be raised when this descriptor is processed.
 *  @hold        : Hold bit is set to hold the next descriptor fetch till the start signal from host.
 *  @opcode      : Gives about type of request(read/write).
 *  @data_len    : Length of data transfer in bytes. 
 *  @eoc	 : End of the command indication bit.
 *  @skip	 : If the descriptor is skipped without filling any fields,then skip bit will be set.
 *  @rsv3        : Reserved bits.
 *  @dma_cmp     : This bit will be set if the descriptor has been successfully processed by DMA controller.
 *  @cmd_tag     : For each command, ID will be provided to identify that particular command.
 *  @ownedByFpga : To set the ownership of the descriptor between FPGA and processor. If this bit is set to 1 by host, FPGA has to handle the 
 descriptor otherwise FPGA will skip to next descriptor.After a particular operation, FPGA will set this bit to 0.
 *  @rsv4        : Reserved bits.
 */
typedef struct csf {
#define DMA_DDR_CSF_OFF    2 /*Bytes*/
	uint32_t   data_len    :18;
	uint32_t   rsv         :14;
	uint32_t   irq_en      : 1;
	uint32_t   hold        : 1;
	uint32_t   opcode      : 1;
	uint32_t   eoc         : 1;
	uint32_t   skip        : 1;
	uint32_t   dma_cmp     : 1;
	uint32_t   cmd_tag     :11;
	uint32_t   ownedByFpga : 1;
	uint32_t   rsv4        :14;
} csf_reg;

/**
 * @Structure   : desc
 * @Brief       : Contains the fields present in the descriptor.
 * @Members     :
 *  @pba         : The ddr3 can be address by filling the row,column and bank fields separately or filling them as a single field.
 *                 So in this union two structure are present.rbc_addr will be used if fields need to be filled separately.Otherwise
 *                 off_addr structure will be used to fill the ddr3 address as single field by incrementing 4KB.
 *  @rsv2        : Reserved bits.
 *  @prp         : 64 bit host address.
 *  @csf          : control and status field structure
 */
typedef struct DdrDmaDesc {
	uint64_t   ppa;
	uint64_t   prp;
	uint32_t   data_len    :18;
	uint32_t   rsv3        :14;
	uint32_t   irq_en      : 1;
	uint32_t   hold        : 1;
	uint32_t   opcode      : 1;
	uint32_t   eoc         : 1;
	uint32_t   skip        : 1;
	uint32_t   dma_cmp     : 1;
	uint32_t   cmd_tag     :11;
	uint32_t   ownedByFpga : 1;
	uint32_t   rsv4        :14;
	uint64_t    rsvd;
#if 0
	uint64_t   ppa;
	uint64_t   prp; 		/* ls2 memmapped addr, dma access triggers iATU addr conversion TODO; watch out for hton type */
	csf_reg      csf;
	uint64_t   rsvd;
#endif    
} DdrDmaDesc;

#endif

