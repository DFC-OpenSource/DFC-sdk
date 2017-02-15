#Script for creating issd images and adding checksum to PBL, DPL, DPC, MC images.

	echo -n "creating issd images "
	sudo rm -rf tmp/deploy/images/ls2085aissd/issd_images
	mkdir -p ./tmp/deploy/images/ls2085aissd/issd_images

#copying the PBL from the directory and adding the version and 
#checksum value at the footer and copying the same to the 
#issd_images directory
	crc=$(crc32 ./tmp/deploy/images/ls2085aissd/rcw/PBL_0x35_0x3F_iSSD.bin)
#        echo "Added checksum value for PBL = $crc"
        c1=$(echo $crc | cut -c1-2)
        c2=$(echo $crc | cut -c3-4)
        c3=$(echo $crc | cut -c5-6)
        c4=$(echo $crc | cut -c7-8)
	sudo echo -ne "03.00.00\x$c1\x$c2\x$c3\x$c4" > temp.bin
	$(cat ./tmp/deploy/images/ls2085aissd/rcw/PBL_0x35_0x3F_iSSD.bin temp.bin > pbl_03.00.00.bin)
	mv pbl_03.00.00.bin tmp/deploy/images/ls2085aissd/issd_images/

#copying the MC from the directory and adding the version and 
#checksum value at the footer and copying the same to the 
#issd_images directory
	crc=$(crc32 ./tmp/deploy/images/ls2085aissd/mc_app/mc.itb)
#        echo "Added checksum value for MC  = $crc"
        c1=$(echo $crc | cut -c1-2)
        c2=$(echo $crc | cut -c3-4)
        c3=$(echo $crc | cut -c5-6)
        c4=$(echo $crc | cut -c7-8)
        sudo echo -ne "03.00.00\x$c1\x$c2\x$c3\x$c4\x00\x00\x00\x00" > temp.bin
        $(cat temp.bin ./tmp/deploy/images/ls2085aissd/mc_app/mc.itb > mc_03.00.00.itb)
	mv mc_03.00.00.itb tmp/deploy/images/ls2085aissd/issd_images/

#copying the DPC from the directory and adding the version and 
#checksum value at the footer and copying the same to the 
#issd_images directory
        crc=$(crc32 ./tmp/deploy/images/ls2085aissd/dpl-examples/dpc-0x2a49.dtb)
#        echo "Added checksum value for DPC = $crc"
        c1=$(echo $crc | cut -c1-2)
        c2=$(echo $crc | cut -c3-4)
        c3=$(echo $crc | cut -c5-6)
        c4=$(echo $crc | cut -c7-8)
        sudo echo -ne "03.00.00\x$c1\x$c2\x$c3\x$c4\x00\x00\x00\x00" > temp.bin
        $(cat temp.bin ./tmp/deploy/images/ls2085aissd/dpl-examples/dpc-0x2a49.dtb > dpc_03.00.00.dtb)
	mv dpc_03.00.00.dtb tmp/deploy/images/ls2085aissd/issd_images/

#copying the DPL from the directory and adding the version and 
#checksum value at the footer and copying the same to the 
#issd_images directory
<<CO
        crc=$(crc32 ./tmp/deploy/images/ls2085aissd/dpl-examples/dpl-eth.0x2A_0x49.dtb)
#        echo "Added checksum value for DPL = $crc"
        c1=$(echo $crc | cut -c1-2)
        c2=$(echo $crc | cut -c3-4)
        c3=$(echo $crc | cut -c5-6)
        c4=$(echo $crc | cut -c7-8)
        sudo echo -ne "03.00.00\x$c1\x$c2\x$c3\x$c4\x00\x00\x00\x00" > temp.bin
        $(cat temp.bin ./tmp/deploy/images/ls2085aissd/dpl-examples/dpl-eth.0x2A_0x49.dtb > dpl_03.00.00.dtb)
CO
    cp ./tmp/deploy/images/ls2085aissd/dpl-examples/dpl-eth.0x2A_0x49.dtb  dpl_03.00.00.dtb
	mv dpl_03.00.00.dtb tmp/deploy/images/ls2085aissd/issd_images/

	$(rm temp.bin)
	echo -n "."

#Creating uImage and copying to the issd_images directory.

	./tmp/work/x86_64-linux/u-boot-mkimage-native/v2015.01+gitAUTOINC+92fa7f53f1-r0/git/tools/mkimage -A arm64 -O linux -T kernel -C none -a 0x80080000 -e 0x80080000 -n "ISSD" -d tmp/deploy/images/ls2085aissd/Image ./tmp/deploy/images/ls2085aissd/issd_images/uimage  > /dev/null
	
#Copying device tree file to the issd_images directory.

	cp tmp/deploy/images/ls2085aissd/Image-fsl-ls2085a-issd.dtb tmp/deploy/images/ls2085aissd/issd_images/device-tree_03.00.00.dtb

#Copying u-boot to the issd_images directory.

	cp tmp/deploy/images/ls2085aissd/u-boot-nor.bin tmp/deploy/images/ls2085aissd/issd_images/u-boot_03.00.00.bin
	
	echo -n "."
#Creating squashfs rootfs and copying to the issd_images directory.
	cp tmp/deploy/images/ls2085aissd/fsl-image-minimal-ls2085aissd.ext2.gz tmp/deploy/images/ls2085aissd/issd_images/
	gunzip tmp/deploy/images/ls2085aissd/issd_images/fsl-image-minimal-ls2085aissd.ext2.gz
	echo -n "."
	mkdir -p tmp/deploy/images/ls2085aissd/issd_images/mnt
	sudo mount -o loop tmp/deploy/images/ls2085aissd/issd_images/fsl-image-minimal-ls2085aissd.ext2 tmp/deploy/images/ls2085aissd/issd_images/mnt/
	echo -n "."
	mkdir -p tmp/deploy/images/ls2085aissd/issd_images/copy
	sudo cp -ar tmp/deploy/images/ls2085aissd/issd_images/mnt/* tmp/deploy/images/ls2085aissd/issd_images/copy/
	echo "$1" | cat - tmp/deploy/images/ls2085aissd/issd_images/copy/etc/version > temp && mv temp tmp/deploy/images/ls2085aissd/issd_images/copy/etc/version
	echo -n "."
	sudo mksquashfs tmp/deploy/images/ls2085aissd/issd_images/copy tmp/deploy/images/ls2085aissd/issd_images/rootfs_03.00.00.sqsh > /dev/null
	echo -n "."
	sudo rm -rf tmp/deploy/images/ls2085aissd/issd_images/copy
	sudo umount tmp/deploy/images/ls2085aissd/issd_images/mnt
	echo -n "."
	sudo rm -rf tmp/deploy/images/ls2085aissd/issd_images/mnt
	rm tmp/deploy/images/ls2085aissd/issd_images/fsl-image-minimal-ls2085aissd.ext2
	echo "."
	echo "Images created successfully in the path : ./tmp/deploy/images/ls2085aissd/issd_images"
