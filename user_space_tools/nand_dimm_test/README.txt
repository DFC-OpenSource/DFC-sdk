
VERSION:
		3.0

FPGA VERSION:
		Supporting FPGA Image VERSIONs are 02.05.02 and a3.03.04.01.

DESCRIPTION :
		The sample_app.c contains the source code  which will do the following:
		1) Erase each block from 2 luns, 2 targets of each NAND chips.
		2) Write to the all pages of block = 0 and block = 1 of all targets, all LUNs in first two chip.
		3) Read back the pages written to NAND and calculating Throughput.

NOTE:
        1) FPGA image version a3.03.04.01 
                a) uses PEX2(FPGA NAND) in Gen3 speed and use M3 slot for the NAND Dimm.
                b) supports Sync mode-2, 2-NAND chips, MultiLUN, MultiTARGET and Multiplane(only for page program) operations.
                c) Sync mode-2 Bandwidth per chip is Read-72MBps and Write-49MBps (Two chips - Read-145MBps Write-98MBps).
        2) FPGA image version 02.05.02 
                a) uses PEX4(FPGA NAND) in Gen2 speed and use M3 & M1 slot for the NAND Dimm.
                b) FPGA image version 02.05.02 supports Async mode-5, 8-NAND chips.
                c) Async mode-5 Bandwitdh per chip is Read-12MBps Write-8MBps (Eight chips - Read-95MBps Write-65MBps).
        3) Upgrade to FPGA image version a3.03.04.01 /*Command to do upgrade: 'upgrade.sh fpga <fpga image.rpd>'*/
		4) Change the Toolchain path in Makefile
		CC = <PATH_TO_BUILD_DIR>/build_ls2085aissd_release/tmp/sysroots/x86_64-linux/usr/bin/aarch64-fsl-linux/aarch64-fsl-linux-gcc --sysroot=<PATH_TO_BUILD_DIR>/build_ls2085aissd_release/tmp/sysroots/ls2085aissd
        5) Copy both application binary and library nand_dm.a to DFC card /run path and execute the application.
		6) To Make the application work for your FPGA and Image version, Read  the BAR 5 address of the corresponding PCI device
				md.l <BAR 5 address>
			the (BAR5_addr + 1c) will show the version of the FPGA image you are using.

		7) Grep the pattern "0xa3030401" in sample_app.c and nand_dm.c. Then change those patterns to the version number you find from step 6.

FOR PERFORMANCE:
		For getting best performance of NAND
		1. Multi-LUN and Multi-target operations by programming parallel pages of all LUNs and Targets of each NAND Chip.
		2. Multi-plane operations by programming parallel pages from both planes block(2n and 2n+1).

