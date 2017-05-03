#CC = /media/vchn144/mem/fslu_nvme_sdk/Layerscape2-SDK-20150515-yocto/build_ls2085aissd_release/tmp/sysroots/x86_64-linux/usr/bin/aarch64-fsl-linux/aarch64-fsl-linux-gcc
CC = /home/sathishkumar/.ssh/fslu_nvme_sdk/Layerscape2-SDK-20150515-yocto/build_ls2085aissd_release/tmp/sysroots/x86_64-linux/usr/bin/aarch64-fsl-linux/aarch64-fsl-linux-gcc
#CC = gcc -g -Wno-packed-bitfield-compat

all:
	$(CC) -pg -c -fPIC -DNEW_DF_CARD params.c  platform.c ioctl.c kmain.c hlm_nobuf.c host_block.c llm_noq.c dm_ramdrive.c dev_ramssd.c ftl/abm.c ftl/page_ftl.c; \
	$(CC) -shared -fPIC -o bdbm.a *.o; \
	chmod a+x bdbm.a;

clean:
	find . -name "*.o" -delete; \
		rm -rf bdbm.a bdbm
