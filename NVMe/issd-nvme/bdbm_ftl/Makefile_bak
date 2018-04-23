# Makefile for BlueDBM Device Driver
#

################################ Flags for using Bluesim ################################
#BOARD = bluesim
################################ Flags for using VC707   ################################
#BOARD = vc707
################################ Flags for using BlueDBM ################################
#BOARD = bluedbm
################################ Flags for using Host    ################################
BOARD = host

ifeq ($(BOARD),bluesim)
HARDWARE_FLAGS = -D BSIM
EXTRA_CFLAGS = -D BOARD_BSIM
BOARDDIR = xbsv/examples/nandsim/$(BOARD)/jni
endif

ifeq ($(BOARD),vc707)
HARDWARE_FLAGS = -D VC707
EXTRA_CFLAGS = -D BOARD_VC707
#BOARDDIR = xbsv/examples/nandsim/$(BOARD)/jni
BOARDDIR = $(BOARD)/jni
endif

ifeq ($(BOARD),bluedbm)
HARDWARE_FLAGS = -D BLUEDBM
EXTRA_CFLAGS = -D BOARD_BLUEDBM
BOARDDIR = bluedbm/jni
endif

#CPPDIR = xbsv/cpp
CPPDIR = connectal/cpp
KBUILD_EXTRA_SYMBOLS := $(PWD)/connectal/drivers/pcieportal/Module.symvers $(PWD)/connectal/drivers/portalmem/Module.symvers

EXTRA_CFLAGS += -D HASH_BLOOM=20 	# for HASH (8KB)
EXTRA_CFLAGS += -D CONFIG_ENABLE_MSG
EXTRA_CFLAGS += -D CONFIG_ENABLE_DEBUG
#EXTRA_CFLAGS += -D SNAPSHOT_ENABLE
#EXTRA_CFLAGS += -D EMULATE_BAD_BLOCKS
EXTRA_CFLAGS += -D USE_PMU
EXTRA_CFLAGS += -D USE_KTIMER

# for connectal
EXTRA_CFLAGS += -D NO_CPP_PORTAL_CODE
EXTRA_CFLAGS += -D __KERNEL__
EXTRA_CFLAGS += -D MMAP_HW
EXTRA_CFLAGS += -I$(PWD)
EXTRA_CFLAGS += -I$(PWD)/connectal
EXTRA_CFLAGS += -I$(PWD)/connectal/cpp
EXTRA_CFLAGS += -I$(PWD)/connectal/drivers/pcieportal
EXTRA_CFLAGS += -I$(PWD)/connectal/drivers/portalmem

bdbm_drv-y := \
	params.o \
	platform.o \
	ioctl.o \
	kmain.o \
	pmu.o \
	host_block.o \
	hlm_nobuf.o \
	hlm_buf.o \
	hlm_rsd.o \
	llm_noq.o \
	llm_mq.o \
	dm_ramdrive.o \
	dev_ramssd.o \
	utils/time.o \
	utils/file.o \
	ftl/abm.o \
	ftl/no_ftl.o \
	ftl/block_ftl.o \
	ftl/page_ftl.o \
	queue/queue.o \
	queue/prior_queue.o

ifeq ($(BOARD),bluesim)
bdbm_drv-y += \
	dm_bluesim.o \
	$(BOARDDIR)/DmaConfigProxy.o \
	$(BOARDDIR)/DmaIndicationWrapper.o \
	$(BOARDDIR)/NandSimIndicationWrapper.o \
	$(BOARDDIR)/NandSimRequestProxy.o \
	$(CPPDIR)/portal.o \
	$(CPPDIR)/dmaManager.o \
	$(CPPDIR)/sock_utils.o
endif

ifeq ($(BOARD),vc707)
#bdbm_drv-y += \
	#dm_bdbme.o \
	#$(BOARDDIR)/DmaConfigProxy.o \
	#$(BOARDDIR)/DmaIndicationWrapper.o \
	#$(BOARDDIR)/NandSimIndicationWrapper.o \
	#$(BOARDDIR)/NandSimRequestProxy.o \
	#$(CPPDIR)/portal.o \
	#$(CPPDIR)/dmaManager.o

bdbm_drv-y += \
	 dm_bluedbm.o \
     $(BOARDDIR)/MMURequest.o \
     $(BOARDDIR)/MMUIndication.o \
     $(BOARDDIR)/FlashIndication.o \
     $(BOARDDIR)/FlashRequest.o \
	 $(BOARDDIR)/MemServerRequest.o \
     $(CPPDIR)/portal.o \
     $(CPPDIR)/dmaManager.o \

endif

ifeq ($(BOARD),bluedbm)
bdbm_drv-y += \
	dm_bluedbm.o \
	$(BOARDDIR)/MMUConfigRequestProxy.o \
	$(BOARDDIR)/MMUConfigIndicationWrapper.o \
	$(BOARDDIR)/FlashIndicationWrapper.o \
	$(BOARDDIR)/FlashRequestProxy.o \
	$(BOARDDIR)/DmaDebugRequestProxy.o \
	$(CPPDIR)/portal.o \
	$(CPPDIR)/dmaManager.o 
endif

obj-m := bdbm_drv.o

ccflags-y := -I$(src)/$(CPPDIR) -I$(src)/$(BOARDDIR) $(HARDWARE_FLAGS) -DBOARD_$(BOARD)

export KROOT=/lib/modules/$(shell uname -r)/build

.PHONY: default
default: modules

.PHONY: modules
modules:
	@$(MAKE) -C $(KROOT) M=$(PWD) modules

.PHONY: modules_check
modules_check:
	@$(MAKE) -C $(KROOT) C=2 M=$(PWD) modules

.PHONY: modules_install
modules_install:
	@$(MAKE) -C $(KROOT) M=$(PWD) modules_install

.PHONY: kernel_clean
kernel_clean:
	@$(MAKE) -C $(KROOT) M=$(PWD) clean
 
.PHONY: clean
clean: kernel_clean
	rm -rf Module.markers modules.order

