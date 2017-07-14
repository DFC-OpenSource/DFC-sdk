
#ifndef _PCI_H
#define _PCI_H

#include "memory.h"

#define PAGE_SIZE 4096
#define PAGE_SHIFT  12    /*Depends on PAGE_SIZE*/
#define MAX_PRP_ENT   512

/**
 * @Structure  : MemoryRegion
 * @Brief      : memory info for local or mapped regions
 * @Members
 *  @addr      : starting address of region
 *  @size      : size of region in bytes
 *  @is_valid  : validation status of the region
 *  			 if 0, addr and size of region are invalid,
 *  			 hence the region is not a valid memory.
 */
typedef struct MemoryRegion {
	uint64_t     addr;
	uint64_t     size;
	uint8_t      is_valid;
} MemoryRegion;

#define PCIE_MAX_EPS		2
#define PCIE_MAX_BARS       6
/**
 * @Structure  : PCIDevice
 * @Brief      : PCI Devices BAR Info
 * @Members
 *  @totalBars : total number of BAR regions exposed by PCI device
 *  @bar       : list of all BARs
 */
typedef struct PCIDevice {
	uint8_t      totalBars;
	MemoryRegion bar[PCIE_MAX_BARS];
} PCIDevice;

/**
 * @Structure  : MappedRegion
 * @Brief      : info about temporarily mapped memory region info
 * @Members
 *  @addr      : starting address of region
 *  @size      : size of region in bytes
 *  @fd        : fd used for mapping memory
 */
typedef struct MappedRegion {
	uint64_t     addr;
	uint64_t     size;
	int          fd;
} MappedRegion;

/**
 * @func   : mapFpgaMem
 * @brief  : map a BAR from FPGA to userspace virtual address
 * @input  : uint64_t bar - BAR base address
 * 			 uint8_t numOfPages - numberof pages the BAR occupies
 * @output : uint64_t *vAddr - pointer to location where mapped BAR address
 * 			 				   is stored for userspace access
 * @return : 0 - success
 * 			 -1 - failure
 */
int mapFpgaMem (uint64_t bar, uint8_t numOfPages, uint64_t *vAddr);

/**
 * @func   : unmapFpgaMem
 * @brief  : unmap a BAR region from userspace virtual address space
 * @input  : uint64_t *vAddr - virtual BAR base address;
 * 							 - assigned with NULL after unmap
 * 			 uint8_t numOfPages - number of pages the BAR occupies
 * @output : none
 * @return : void
 */
void unmapFpgaMem (uint64_t *vAddr, uint8_t numOfPages);

/**
 * @func   : mmap_prp_addr
 * @brief  : get mapped address for hosts prps from prp list;
 * 			 used in test setup (host/ctrl on same system)
 * 			 where the addr is physical address from host driver
 * 			 and ctrl needs to get virtual address for that addr
 * @input  : uint64_t base_addr - base address
 * 			 uint64_t off_addr - offset address for prp
 * 			 uint8_t nPages - number of pages
 * 			 int *fd - fd for dev-mem for mmap access
 * @output : if *fd = 0, fd is assigned with descriptor value 
 * 			 after dev-mem node is opened
 * @return : NULL - failure (fd open or mmap failed)
 * 			 Non-NULL - valid mapped address
 */
uint64_t mmap_prp_addr (uint64_t base_addr, uint64_t off_addr, uint8_t nPages, int *fd, int ns_num);

/**
 * @func   : mmap_queue_addr
 * @brief  : get mapped address for hosts SQ|CQ;
 * 			 used in test setup (host/ctrl on same system)
 * 			 where the addr is physical address from host driver
 * 			 and ctrl needs to get virtual address for that addr
 * @input  : uint64_t host_addr - base address of host
 * 			 uint64_t prp - address of queue
 * 			 uint16_t qid - index of Queue
 * 			 int *fd - fd pointer to store fd for DEV_MEM
 * @output : none
 * @return : 0 - failure (fd open or mmap failed)
 * 			 non-zero - valid mapped address
 */
uint64_t mmap_queue_addr (uint64_t host_base, uint64_t prp, uint16_t qid, int *fd);

/**
 * @func   : unmapHostMem
 * @brief  : unmap hosts memory from nvme userspace virtual address space;
 * 			 used in test setup (host/ctrl on same system)
 * 			 where the addr is physical address from host driver
 * @input  : uint64_t *vAddr - virtual BAR base address;
 * 							 - assigned with NULL after unmap
 * 			 uint8_t numOfPages - number of pages the BAR occupies
 * @output : none
 * @return : void
 */
void unmapHostMem (uint64_t *vAddr, uint8_t numOfPages);

#endif /*#ifndef _PCI_H*/

