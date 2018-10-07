
#ifndef __QDMA_USER_SPACE_LIB_H
#define __QDMA_USER_SPACE_LIB_H

#include<stdio.h> 
#include<stdint.h> 
#include<sys/types.h> 
#include<sys/stat.h> 
#include<fcntl.h> 
#include<string.h> 
#include<sys/mman.h> 
#include<unistd.h> 
#include<stdlib.h>
#include<pthread.h> 

#define QDMA_DESC_COUNT 128
#define QDMA_CHANNELS_COUNT 8


#define DMA_START 0x1
#define DMA_GOING 0x2
#define DMA_DONE  0x3
#define DMA_FREE  0x4

enum MAP_STATUS {
	NOT_MAPPED,
	MMAP_SUCCESS,
	MMAP_FAILED    
};

enum FD_STATUS {
	FD_SUCCESS,
	FD_FAILED
};

#define dsb(opt)        asm volatile("dsb " #opt : : : "memory")
#define wmb()           dsb(st)


struct qdma_desc {
	volatile uint64_t src_addr;
	volatile uint64_t dest_addr;
	volatile uint32_t len;
	volatile uint32_t status;
};

void __attribute__ ((constructor)) setup_dma_mmaps(void);
struct qdma_desc *qdma_transfer(int channel_no,uint64_t src,uint64_t dest, uint32_t len);
int qdma_desc_release(volatile struct qdma_desc *desc);
int display_desc_status(int channel_no);
#endif
