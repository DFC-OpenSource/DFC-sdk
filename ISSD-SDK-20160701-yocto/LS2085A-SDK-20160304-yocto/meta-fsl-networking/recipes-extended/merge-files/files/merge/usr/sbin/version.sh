if [ "$1" = "checksum" ]
then
	echo " ==============================================================="
	echo "	   Image		Version		     Checksum         "
	echo " ==============================================================="
else
	echo " ========================================"
	echo "	Image		Version	               "
	echo " ========================================"
fi
cd /run/.
#PBL Version and checksum reading
$(hexdump /dev/mtd0 | head -n 3 > tmp)
pbi_checksum=$(sed -n 3p tmp | cut -c34-37)
offset=$((0x$pbi_checksum - 0xc1))
offset=$(($offset / 0x4))
offset=$(($offset + 0xbc))
$(split /dev/mtd0 -b $offset)
$(hexdump -C xab | head -n 1 > tmp)
c1=$(cat tmp | cut -c36-37)
c2=$(cat tmp | cut -c39-40)
c3=$(cat tmp | cut -c42-43)
c4=$(cat tmp | cut -c45-46)
c5=$(cat tmp | cut -c62-69)
$(rm -rf x*)

if [ "$1" = "checksum" ] 
then
	echo "	  PBL 			$c5		 $c1$c2$c3$c4"
else
	echo "    PBL                   $c5"
fi
<<CO
#DPL Version and checksum reading
$(hexdump -C /dev/mtd3 | head -n 1 > tmp)
c1=$(cat tmp | cut -c36-37)
c2=$(cat tmp | cut -c39-40)
c3=$(cat tmp | cut -c42-43)
c4=$(cat tmp | cut -c45-46)
c5=$(cat tmp | cut -c62-69)

if [ "$1" = "checksum" ] 
then
	echo "	  DPL 			$c5		 $c1$c2$c3$c4"
else
	echo "    DPL                   $c5"
fi
CO
#DPC Version and checksum reading
$(hexdump -C /dev/mtd4 | head -n 1 > tmp)
c1=$(cat tmp | cut -c36-37)
c2=$(cat tmp | cut -c39-40)
c3=$(cat tmp | cut -c42-43)
c4=$(cat tmp | cut -c45-46)
c5=$(cat tmp | cut -c62-69)

if [ "$1" = "checksum" ] 
then
	echo "	  DPC 			$c5		 $c1$c2$c3$c4"
else
	echo "    DPC                   $c5"
fi

#MC Version and checksum reading
$(hexdump -C /dev/mtd5 | head -n 1 > tmp)
c1=$(cat tmp | cut -c36-37)
c2=$(cat tmp | cut -c39-40)
c3=$(cat tmp | cut -c42-43)
c4=$(cat tmp | cut -c45-46)
c5=$(cat tmp | cut -c62-69)

if [ "$1" = "checksum" ] 
then
	echo "	  MC 			$c5		 $c1$c2$c3$c4"
else
	echo "    MC                    $c5"
fi

$(rm tmp)

#Kernel Version and checksum reading
var=$(cat /proc/kernel_ver)
c5=${var#*:}

if [ "$1" = "checksum" ] 
then
	echo "	  Kernel 	       $c5		  --   "
else
	echo "    Kernel               $c5"
fi

#U-boot Version and checksum reading
var1=$(cat /proc/cmdline)
var2=${var1##*ubootver=}
echo $var2 > tmp
var3=$(cat tmp | cut -c1-8)
if [ "$1" = "checksum" ] 
then
	echo "	  U-boot 	        $var3		  --   "
else
	echo "    U-boot                $var3"
fi

#rootfs version and checksum reading
c5=$(cat /etc/version | head -n 1 | cut -c1-8)

if [ "$1" = "checksum" ] 
then
	echo "	  rootfs 		$c5		  --   "
else
	echo "    rootfs                $c5"
fi

#cpld version reading
c5=$(i2capp cpld_nic version)
if [ "$1" = "checksum" ] 
then
	echo "	  CPLD_NIC	 	$c5		  	  --   "
else
	echo "    CPLD_NIC              $c5"
fi

#cpld_ssd
var1=$(i2capp cpld_ssd version)
if [ "$1" = "checksum" ] 
then
	echo "	  CPLD_SSD	        $var1		  	  --   "
else
	echo "    CPLD_SSD              $var1"
fi

#FPGA version
c5=$(fpga_version)
if [ "$1" = "checksum" ] 
then
	echo "	  FPGA		 	$c5		  --   "
else
	echo "    FPGA		  $c5"
fi

cd - > /dev/null
