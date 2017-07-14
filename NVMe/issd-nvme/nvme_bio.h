
#ifndef __NVME_BIO_H
#define __NVME_BIO_H

#include <stdint.h>


//#define CONTINUOUS_PRP

/**
 * @Structure  : nvme_bio.
 * @Brief      : Contains the parameters used for I/O operation between host and storage DDR.
 * @Members
 *  @req_info  : void * pointing to nvme request structure
 *  @mem_type  : memory target type - ramdisk|ddr|nand
 *  @slba      : Starting logical block address
 *  @nlb       : Number of required logical blocks
 *  @size      : Total size of data for bio operation
 *  @req_type  : Request type(read/write)
 *  @nprps     : number of prp's. Ranges 1 to maximum specified by bio requestor.
 *  @prp       : Contains the prp's which come under this request.
 */
typedef struct nvme_bio {
	void     *req_info;
	uint64_t slba;
	//uint64_t n_sectors;
	uint64_t nlb;
	uint64_t size;
	uint8_t ns_num;
	uint32_t req_type;
	uint16_t nprps;
	uint16_t offset;
	uint64_t prp[128];
	uint8_t is_seq;
	uint8_t data_shift;
#ifdef DDR_CACHE
	uint64_t f_lba;
#endif
} nvme_bio;

/**
 * @func   : nvme_bio_request
 * @brief  : request block IO to FTL with necessary addr & size info
 * @input  : nvme_bio *bio - pointer to bio request structure
 * @output : none
 * @return : 0 - success
 * 			 < 0 - failure
 */
int nvme_bio_request (nvme_bio *bio);

/**
 * @Func	: validate_lba
 * @Brief	: Check wheather the starting logical block address(slba) and 
 *		  number of logical blocks(nlb) are within its range.Based on the 
 *		  result of this validation,the DMA Adesccriptor creation thread 
 *		  will start its operation.
 *  @input	: nvme_bio - Structure from which the slba and nlb will be extracted 
 *		  	     for the validation process.It contains the parameters 
 *			     used for I/O operation between x86 and storage DDR.
 *  @return 	: return zero on success and non zero on failure of validation.
 */
int validate_lba (nvme_bio);

static inline int nvme_add_prp (nvme_bio *bio, uint64_t prp, uint64_t trans_len)
{
	bio->prp[bio->nprps] = prp;
	bio->size += trans_len;
	++bio->nprps;
	return 0;
}

#if !GC_THREAD_ENABLED

#define nvme_bio_allow_gc() (0)

#else

static inline int nvme_bio_allow_gc (void)
{
	return host_block_allow_gc (_bdi);
}

#endif /*!GC_THREAD_ENABLED*/
extern inline void ramdisk_prp_rw (void *prp, uint64_t ramdisk_addr, uint32_t req_type, uint64_t len);
int nvme_add_prp (nvme_bio *bio, uint64_t prp, uint64_t trans_len);
inline int nvme_bio_mmap_prps (nvme_bio *bio);

#endif /*#ifndef __NVME_BIO_H*/

