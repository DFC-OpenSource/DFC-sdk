# Makefile for nvme controller application

TARGET = issd_nvme
CFLAGS = -pthread
CFLAGS += -lrt
CFLAGS += -pg
CFLAGS += -g
#CFLAGS += -Wall
CFLAGS += -D_GNU_SOURCE
CFLAGS += -Wno-packed-bitfield-compat
#CFLAGS += -Wno-unused-function
SRCS=*.c
FTL= bdbm_ftl/params.c  bdbm_ftl/platform.c bdbm_ftl/ioctl.c bdbm_ftl/kmain.c bdbm_ftl/hlm_nobuf.c bdbm_ftl/host_block.c bdbm_ftl/llm_noq.c bdbm_ftl/dm_dragonfire.c bdbm_ftl/dev_dragonfire.c bdbm_ftl/ftl/abm.c bdbm_ftl/ftl/page_ftl.c

#CC = gcc -g -Wno-packed-bitfield-compat
#Cross compilation command
#CC = /media/vvdn/nvme/fslu_nvme_sdk_ear5/Layerscape2-SDK-20150828-yocto/build_ls2085aissd_release/tmp/sysroots/x86_64-linux/usr/bin/aarch64-fsl-linux/aarch64-fsl-linux-gcc --sysroot=/media/vvdn/nvme/fslu_nvme_sdk_ear5/Layerscape2-SDK-20150828-yocto/build_ls2085aissd_release/tmp/sysroots/ls2085aissd
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

ls2-x86: validate_rdinfo
	$(CC) $(SRCS) $(FTL) $(CFLAGS) -DCUR_SETUP=TARGET_RD_SETUP \
		-DRAMDISK_PATH=\"$(rdp)\" -DRAMDISK_SZ_MB=$(sz) -o $(TARGET)

ftl-rd: BDBM_FTL
	$(CC) $(SRCS) ./bdbm.a $(CFLAGS) -DCUR_SETUP=TARGET_RD_SETUP -o $(TARGET)

x86: validate_rdinfo
	$(CC) $(SRCS) $(CFLAGS) -DCUR_SETUP=X86_TEST_SETUP \
		-DRAMDISK_PATH=\"$(rdp)\" -DRAMDISK_SZ_MB=$(sz) -o $(TARGET)

ls2: validate_rdinfo
	$(CC) $(SRCS) $(CFLAGS) -DCUR_SETUP=LS2_TEST_SETUP \
		-DRAMDISK_PATH=\"$(rdp)\" -DRAMDISK_SZ_MB=$(sz) -o $(TARGET)

validate_rdinfo:
ifndef rdp
	$(error "No Ramdisk Path specified!")
	echo $(help_str)
endif
ifndef sz
	$(error "No Ramdisk Size specified!")
	echo $(help_str)
endif

fslustdalone:
	$(CC) $(SRCS) $(FTL) $(CFLAGS) -DCUR_SETUP=STANDALONE_SETUP -DBIO_MEMTYPE=NVME_BIO_DDR -o issd_nvme_stda

fsluddr:
	$(CC) $(SRCS) $(FTL) $(CFLAGS) -DCUR_SETUP=TARGET_SETUP -DBIO_MEMTYPE=NVME_BIO_DDR -o $(TARGET)

fslunand:
	$(CC) $(SRCS) $(FTL) $(CFLAGS) -DCMD_STATUS_DESC -DNEW_DF_CARD -DCUR_SETUP=TARGET_SETUP -DBIO_MEMTYPE=NVME_BIO_NAND -o $(TARGET)

fsluissd:
	$(CC) $(SRCS) $(FTL) $(CFLAGS) -DGET_CMD_STATUS -DCUR_SETUP=TARGET_SETUP -DBIO_MEMTYPE=NVME_BIO_TWO -o $(TARGET)

clean:
	rm -rf *.o $(TARGET)

