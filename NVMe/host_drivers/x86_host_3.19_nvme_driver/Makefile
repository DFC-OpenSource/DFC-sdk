#nvme-core-objs=nvme-core.o nvme-scsi.o 
obj-m = nvme.o
nvme-y = nvme-core.o nvme-scsi.o
KVERSION = $(shell uname -r)
all:
	make -C /lib/modules/$(KVERSION)/build M=$(shell pwd) modules
#	make -C /lib/modules/$(KVERSION)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(KVERSION)/build M=$(shell pwd) clean
#	make -C /lib/modules/$(KVERSION)/build M=$(PWD) clean
