
#CROSS_COMPILE ?=
#CC=$(CROSS_COMPILE)gcc

flash_fpga:
	$(CC) -o flash_fpga app.c -lpthread
clean:
	rm -f flash_fpga*
	rm -f fpga_version*

