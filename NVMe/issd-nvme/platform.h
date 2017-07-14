
#ifndef __PLATFORM_H
#define __PLATFORM_H

/* A script might come up for init sequence of ./nvme
 * which will allow the user to define various platform specific macros*/

#define TARGET_SETUP     	0x0    /*HOST--CARD*/
#define STANDALONE_SETUP     	0x4    /*HOST--CARD*/

#if (CUR_SETUP != STANDALONE_SETUP)
/*#define DDR_CACHE*/
#endif
#define QDMA 0x1	/* QDMA*/
#define NVME_BIO_RAMDISK 0x01
#define NVME_BIO_DDR     0x02
#define NVME_BIO_NAND    0x04
#define NVME_BIO_TWO   	 0x08	/*DDR+NAND*/

#ifndef CUR_SETUP
#error "*****NEED TO DEFINE CUR_SETUP TO BUILD THE NVMe APP FOR A SPECIFIC SETUP*****"
#endif

#if (!defined(BIO_MEMTYPE) || ((BIO_MEMTYPE != NVME_BIO_NAND) && (BIO_MEMTYPE != NVME_BIO_DDR) && (BIO_MEMTYPE != NVME_BIO_TWO)))
#error "*****NEED TO DEFINE VALID BIO_MEMTYPE TO BUILD THE NVMe APP FOR TARGET SETUP*****"
#endif

#endif /*#ifndef __PLATFORM_H*/

