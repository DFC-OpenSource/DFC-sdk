TARGET = sample_app

#CC = <PATH_TO_BUILD_DIR>/build_ls2085aissd_release/tmp/sysroots/x86_64-linux/usr/bin/aarch64-fsl-linux/aarch64-fsl-linux-gcc --sysroot=<PATH_TO_BUILD_DIR>/build_ls2085aissd_release/tmp/sysroots/ls2085aissd
#CC = /media/vchn144/mem/fslu_nvme_sdk/Layerscape2-SDK-20150515-yocto/build_ls2085aissd_release/tmp/sysroots/x86_64-linux/usr/bin/aarch64-fsl-linux/aarch64-fsl-linux-gcc
#CC = gcc
CC = /home/vchn181/SDK/ls88/fslu_nvme_ls2088sdkv3/LS2088A-SDK-20161223-yocto/build_ls2088aissd/tmp/sysroots/x86_64-linux/usr/bin/aarch64-fsl-linux/aarch64-fsl-linux-gcc --sysroot=/home/vchn181/SDK/ls88/fslu_nvme_ls2088sdkv3/LS2088A-SDK-20161223-yocto/build_ls2088aissd/tmp/sysroots/ls2088aissd
CFLAGS += -pthread
CFLAGS += -lrt
CFLAGS += -g

all:
	$(CC) -pg -c -fPIC nand_dma.c; \
	$(CC) -shared -fPIC -o nand_dm.a *.o; \
	chmod a+x nand_dm.a;
	$(CC) sample_app.c ./nand_dm.a $(CFLAGS) -o $(TARGET) 

clean:
	find . -name "*.o" -delete; \
		rm -rf nand_dm.a $(TARGET)
