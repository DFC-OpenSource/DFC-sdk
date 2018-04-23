
VERSION:
		3.1

FPGA VERSION:
		Supporting all the FPGA Image VERSIONs(Version_check is removed) 

DESCRIPTION :
		The sample_app.c contains the source code  which will do the following:
		1) Erase each block from 2 luns, 2 targets of each NAND chips.
		2) Write to the all pages of block = 0 and block = 1 of all targets, all LUNs in first two chip.
		3) Read back the pages written to NAND and calculating Throughput.

NOTE:
    	1)Upgrade to FPGA image version a3.03.05.01 /*Command to do upgrade: 'upgrade.sh fpga <fpga image.rpd>'*/
		2) Change the Toolchain path in Makefile
		CC = <PATH_TO_BUILD_DIR>/build_ls2085aissd_release/tmp/sysroots/x86_64-linux/usr/bin/aarch64-fsl-linux/aarch64-fsl-linux-gcc --sysroot=<PATH_TO_BUILD_DIR>/build_ls2085aissd_release/tmp/sysroots/ls2085aissd
        3) Copy both application binary and library nand_dm.a to DFC card /run path and execute the application.

FOR PERFORMANCE:
		For getting best performance of NAND
		1. Multi-LUN and Multi-target operations by programming parallel pages of all LUNs and Targets of each NAND Chip.
		2. Multi-plane operations by programming parallel pages from both planes block(2n and 2n+1).
