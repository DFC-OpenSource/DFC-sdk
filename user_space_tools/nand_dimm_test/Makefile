TARGET = nand_chipwise_test

CC = <Path to SDK>/LS2085A-SDK-20160304-yocto/build_ls2085aissd_release/tmp/sysroots/x86_64-linux/usr/bin/aarch64-fsl-linux/aarch64-fsl-linux-gcc --sysroot=<Path to SDK>/LS2085A-SDK-20160304-yocto/build_ls2085aissd_release/tmp/sysroots/ls2085aissd

CFLAGS = -pthread
CFLAGS += -lrt
#CFLAGS += -g

all:
	$(CC) -pg -c -fPIC nand_dma.c; \
	$(CC) -shared -fPIC -o nand_dm.a *.o; \
	chmod a+x nand_dm.a;
	$(CC) -g sample_app.c ./nand_dm.a $(CFLAGS) -o $(TARGET) 

clean:
	find . -name "*.o" -delete; \
		rm -rf nand_dm.a $(TARGET)
