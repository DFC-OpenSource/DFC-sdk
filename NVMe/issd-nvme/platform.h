
#ifndef __PLATFORM_H
#define __PLATFORM_H

/* A script might come up for init sequence of ./nvme
 * which will allow the user to define various platform specific macros*/

#define TARGET_SETUP     	0x0    /*HOST--CARD*/
#define X86_TEST_SETUP   	0x1    /*HOST--FPGA*/
#define LS2_TEST_SETUP   	0x2    /*LS2--FPGA*/
#define TARGET_RD_SETUP     	0x3    /*HOST--LS2--FPGA*/
#define STANDALONE_SETUP     	0x4    /*HOST--CARD*/

#define NVME_BIO_RAMDISK 0x01
#define NVME_BIO_DDR     0x02
#define NVME_BIO_NAND    0x04
#define NVME_BIO_TWO   	 0x08	/*DDR+NAND*/

#ifndef CUR_SETUP
#error "*****NEED TO DEFINE CUR_SETUP TO BUILD THE NVMe APP FOR A SPECIFIC SETUP*****"
#endif

#if (CUR_SETUP != TARGET_SETUP && CUR_SETUP != STANDALONE_SETUP)

#ifndef RAMDISK_PATH
#error "*****NEED TO DEFINE RAMDISK_PATH TO BUILD THE NVMe APP FOR TEST SETUPS*****"
#endif

#ifndef RAMDISK_SZ_MB
#error "*****NEED TO DEFINE RAMDISK_SZ_MB TO BUILD THE NVMe APP FOR TEST SETUPS*****"
#endif

#else /*TARGET_SETUP*/

#if (!defined(BIO_MEMTYPE) || ((BIO_MEMTYPE != NVME_BIO_NAND) && (BIO_MEMTYPE != NVME_BIO_DDR) && (BIO_MEMTYPE != NVME_BIO_TWO)))
#error "*****NEED TO DEFINE VALID BIO_MEMTYPE TO BUILD THE NVMe APP FOR TARGET SETUP*****"
#endif

#endif /*#if (CUR_SETUP != TARGET_SETUP)*/

#endif /*#ifndef __PLATFORM_H*/

