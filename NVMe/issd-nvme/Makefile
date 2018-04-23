# Makefile for nvme controller application

TARGET = issd_nvme
CFLAGS = -pthread
CFLAGS += -lrt
CFLAGS += -pg
CFLAGS += -g
CFLAGS += -lqdma
CFLAGS += -O0
#CFLAGS += -Wall
CFLAGS += -D_GNU_SOURCE
CFLAGS += -Wno-packed-bitfield-compat
#CFLAGS += -Wno-unused-function
SRCS=*.c

TARGET1 = flash_mgnt_app
CFLAGS1 = -pthread
CFLAGS1 += -lrt
CFLAGS1 += -D_GNU_SOURCE
SRCS1=  sd_card_app/msg_app.c

FTL= bdbm_ftl/params.c  bdbm_ftl/platform.c bdbm_ftl/ioctl.c bdbm_ftl/kmain.c bdbm_ftl/hlm_nobuf.c bdbm_ftl/host_block.c bdbm_ftl/llm_noq.c bdbm_ftl/dm_dragonfire.c bdbm_ftl/dev_dragonfire.c bdbm_ftl/ftl/abm.c bdbm_ftl/ftl/page_ftl.c
#CC = gcc -g -Wno-packed-bitfield-compat
##Cross compilation command for EAR5/EAR6
CC= /home/vchn181/SDK/ls88/fslu_nvme_ls2088sdkv3/LS2088A-SDK-20161223-yocto/build_ls2088aissd/tmp/sysroots/x86_64-linux/usr/bin/aarch64-fsl-linux/aarch64-fsl-linux-gcc --sysroot=/home/vchn181/SDK/ls88/fslu_nvme_ls2088sdkv3/LS2088A-SDK-20161223-yocto/build_ls2088aissd/tmp/sysroots/ls2088aissd
help_str="\nUsage: make <setup>"\
		 "\n\tfslustdalone: standalone setup - ls2 NVMe host & controller- DDR Namespace"\
		 "\n\tfsluissd: target setup - x86 host & ls2 NVMe - Two Namespaces - DDR & NAND"\
		 "\n\tfsluddr: target setup - x86 host & ls2 NVMe - DDR Namespace"\
		 "\n\tfslunand: target setup - x86 host & ls2 NVMe - NAND Namespace"\
		 "\n\tx86: test setup - host & NVMe on x86 - ramdisk Namespace"\
		 "\n\tls2: test setup - host & NVMe on ls2 - ramdisk Namespace"\
		 "\n\tls2-x86: test setup - host on x86 & NVMe on ls2 - ramdisk Namespace"\
		 "\n\n\"x86\" & \"ls2\" targets need make arguments:"\
		 "rdp=\"<ramdisk-path>\" sz=<ramdisk-size-in-MB-count>"
+                "Example: make ls2-x86 rdp=\"path\" sz=0x200000000"
help:
	echo $(help_str)

fslustdalone:
	$(CC) $(SRCS) $(FTL) $(CFLAGS) -L ./ -DGET_CMD_STATUS -DCUR_SETUP=STANDALONE_SETUP -DBIO_MEMTYPE=NVME_BIO_TWO libqdma.so -o issd_nvme_stda
	$(CC) $(SRCS1) -o $(TARGET1) $(CFLAGS1)

fsluddr:
	$(CC) $(SRCS) $(FTL) $(CFLAGS) -DCUR_SETUP=TARGET_SETUP -DBIO_MEMTYPE=NVME_BIO_DDR -o $(TARGET)
	$(CC) $(SRCS1) -o $(TARGET1) $(CFLAGS1)

fslunand:
	$(CC) $(SRCS) $(FTL) $(CFLAGS) -DCMD_STATUS_DESC -DNEW_DF_CARD -DCUR_SETUP=TARGET_SETUP -DBIO_MEMTYPE=NVME_BIO_NAND -o $(TARGET)
	$(CC) $(SRCS1) -o $(TARGET1) $(CFLAGS1)

fsluissd:
	$(CC) $(SRCS) $(FTL) $(CFLAGS) -L ./ -DCUR_SETUP=TARGET_SETUP -DBIO_MEMTYPE=NVME_BIO_TWO libqdma.so -o $(TARGET)
	$(CC) $(SRCS1) -o $(TARGET1) $(CFLAGS1)

clean:
	rm -rf *.o $(TARGET) $(TARGET1)
