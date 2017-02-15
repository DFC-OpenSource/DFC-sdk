# Filename   :   flash_images.sh
#
# Author     :   
#
# Description:   Script for flashing binary into iNIC card.

ROOT_DIR=$1
USER_OPT=`echo $2 | tr "[:upper:]" "[:lower]"`
BINARY_PATH=$ROOT_DIR/images

PBL_KEY="pbl"
UBOOT_KEY="u-boot"
DPL_KEY="dpl"
DPC_KEY="dpc"
MC_KEY="mc"
DTB_KEY="device"
UIMAGE_INIC_KEY="uimage_inic"
UIMAGE_ISSD_KEY="uimage_issd"
ROOTFS_KEY="rootfs"
FPGA_KEY="fpga"

PBL_FILE_NAME=""
UBOOT_FILE_NAME=""
DPL_FILE_NAME=""
DPC_FILE_NAME=""
MC_FILE_NAME=""
DTB_FILE_NAME=""
UIMAGE_INIC_FILE_NAME=""
UIMAGE_ISSD_FILE_NAME=""
ROOTFS_FILE_NAME=""
FPGA_FILE_NAME=""
PBL_FILE_VER=""
PBL=""
UBOOT_FILE_VER=""
UBOOT=""
DPL_FILE_VER=""
DPL=""
DPC_FILE_VER=""
DPC=""
MC_FILE_VER=""
MC=""
DTB_FILE_VER=""
DTB=""
UIMAGE_INIC_FILE_VER=""
UIMAGE_INIC=""
UIMAGE_ISSD_FILE_VER=""
UIMAGE_ISSD=""
ROOTFS_FILE_VER=""
ROOTFS=""
FPGA_FILE_VER=""
FPGA=""

if [ $USER_OPT = "factory_default" ]; then
PBL_PARTITION=/dev/mtd0
UBOOT_PARTITION=/dev/mtd1
UBOOT_ENV_PARTITION=/dev/mtd2
DPL_PARTITION=/dev/mtd3
DPC_PARTITION=/dev/mtd4
MC_PARTITION=/dev/mtd5
DTB_PARTITION=/dev/mtd6
UIMAGE_INIC_PARTITION=/dev/mtd7
UIMAGE_ISSD_PARTITION=/dev/mtd8
ROOTFS_PARTITION=/dev/mtd9
DEBUG_LOG_PARTITION=/dev/mtd10
else
PBL_PARTITION=/dev/mtd11
UBOOT_PARTITION=/dev/mtd12
UBOOT_ENV_PARTITION=/dev/mtd13
DPL_PARTITION=/dev/mtd14
DPC_PARTITION=/dev/mtd15
MC_PARTITION=/dev/mtd16
DTB_PARTITION=/dev/mtd17
UIMAGE_INIC_PARTITION=/dev/mtd18
UIMAGE_ISSD_PARTITION=/dev/mtd19
ROOTFS_PARTITION=/dev/mtd20
DEBUG_LOG_PARTITION=/dev/mtd21
fi

function init
{
	PBL_FILE_NAME=$(find $BINARY_PATH -iname '*pbl*'|awk -F/ '{print $NF}')
	UBOOT_FILE_NAME=$(find $BINARY_PATH -iname '*u-boot*'|awk -F/ '{print $NF}')
	DPL_FILE_NAME=$(find $BINARY_PATH -iname '*dpl*'|awk -F/ '{print $NF}')
	DPC_FILE_NAME=$(find $BINARY_PATH -iname '*dpc*'|awk -F/ '{print $NF}')
	MC_FILE_NAME=$(find $BINARY_PATH -iname '*mc*'|awk -F/ '{print $NF}')
	DTB_FILE_NAME=$(find $BINARY_PATH -iname '*device*'|awk -F/ '{print $NF}')
	UIMAGE_INIC_FILE_NAME=$(find $BINARY_PATH -iname '*uimage_inic*'|awk -F/ '{print $NF}')
	UIMAGE_ISSD_FILE_NAME=$(find $BINARY_PATH -iname '*uimage_issd*'|awk -F/ '{print $NF}')
	ROOTFS_FILE_NAME=$(find $BINARY_PATH -iname '*rootfs*'|awk -F/ '{print $NF}')
	FPGA_FILE_NAME=$(find $BINARY_PATH -iname '*fpga*'|awk -F/ '{print $NF}')

	PBL=${PBL_FILE_NAME%.*};
	PBL_FILE_VER=${PBL##*_};
	PBL=${PBL_FILE_NAME%_*};

	UBOOT=${UBOOT_FILE_NAME%.*};
	UBOOT_FILE_VER=${UBOOT##*_};
	UBOOT=${UBOOT_FILE_NAME%_*};

	DPL=${DPL_FILE_NAME%.*};
	DPL_FILE_VER=${DPL##*_};
	DPL=${DPL_FILE_NAME%_*};

	DPC=${DPC_FILE_NAME%.*};
	DPC_FILE_VER=${DPC##*_};
	DPC=${DPC_FILE_NAME%_*};

	MC=${MC_FILE_NAME%.*};
	MC_FILE_VER=${MC##*_};
	MC=${MC_FILE_NAME%_*};

	DTB=${DTB_FILE_NAME%.*};
	DTB_FILE_VER=${DTB##*_};
	DTB=${DTB_FILE_NAME%_*};

	UIMAGE_INIC=${UIMAGE_INIC_FILE_NAME%.*};
	UIMAGE_INIC_FILE_VER=${UIMAGE_INIC##*_};
	UIMAGE_INIC=${UIMAGE_INIC_FILE_NAME%_*};

	UIMAGE_ISSD=${UIMAGE_ISSD_FILE_NAME%.*};
	UIMAGE_ISSD_FILE_VER=${UIMAGE_ISSD##*_};
	UIMAGE_ISSD=${UIMAGE_ISSD_FILE_NAME%_*};

	ROOTFS=${ROOTFS_FILE_NAME%.*};
	ROOTFS_FILE_VER=${ROOTFS##*_};
	ROOTFS=${ROOTFS_FILE_NAME%_*};
	
	FPGA=${FPGA_FILE_NAME%.*};
	FPGA_FILE_VER=${FPGA##*_};
	FPGA=${FPGA_FILE_NAME%_*};
}

function disply
{
	echo "       _     ____   ___    _             _      "       
	echo "      | |___|___ \ / _ \  (_)___ ___  __| |     "    
	echo "      | / __| __) | | | | | / __/ __|/ _  |     " 
	echo "      | \__ \/ __/| |_| | | \__ \__ \ (_| |     "
	echo "      |_|___/_____|\___/  |_|___/___/\__,_|     "

	echo "------------------------------------------------"
	echo "            I M A G E  U P G R A D E            "
	echo "------------------------------------------------"
}

function display_fpga
{

	echo "             _____ ____   ____    _              "    
	echo "            |  ___|  _ \ / ___|  / \             "   
	echo "            | |_  | |_) | |  _  / _ \            "  
	echo "            |  _| |  __/| |_| |/ ___ \           "
	echo "            |_|   |_|    \____/_/   \_\          "
 	echo "-------------------------------------------------"
        echo "              I M A G E  U P G R A D E           "
        echo "-------------------------------------------------"
}

function display_shutdown
{
	echo "-------------------------------------------------"
        echo "       Please shutdown the host machine          "
        echo "            and switch it ON again               "
        echo "-------------------------------------------------"        
}

function display_success
{
	name=$1
        echo "------------------------------------------------"
        echo "   $name updated successfully                   " 
        echo "------------------------------------------------"        
}

function display_binary_exist_msg
{
        name=$1
        echo "------------------------------------------------"
        echo "     $name is already updated                   " 
        echo "------------------------------------------------"        
}

function print_error
{
	err_no=$1
        arg=$2
        case $err_no in
        	1) echo "ERROR: $arg does not exist"; exit 1;;
                2) echo "ERROR: multiple $arg binary exist"; exit 1;;
                3) echo "ERROR: $arg binary does not exist"; exit 1;;
                4) echo "ERROR: Incorrect $arg file name"; exit 1;;
                5) echo "ERROR: $arg update failed"; exit 1;;
                6) echo "ERROR: Checksum failed data corrupted";;
                *) echo "Error: Invalid error type"
        esac
}

function check_binary
{
	KEY=$1
        FILE=$2
 	VER=$3
        EXT=$4
  
        number_bin=$(ls $BINARY_PATH | grep -i $KEY | wc -l)

        if [ "$number_bin" == "0" ]; then
                print_error 3 $KEY
        elif [ "$number_bin" != "1" ]; then
                print_error 2 $KEY
        elif [ ! -f "$BINARY_PATH/${FILE}_${VER}.${EXT}" ]; then
                print_error 4 $KEY
        fi
}

function validate_binaries
{
	if [ ! -d $BINARY_PATH ]; then
        echo "ERROR: images directory does not exist"
        exit 1
	fi

        for num in {1..10}
        do
                if [ $num = 1 ]; then
                        check_binary $PBL_KEY $PBL $PBL_FILE_VER "bin"
                elif [ $num = 2 ]; then
                      	check_binary $UBOOT_KEY $UBOOT $UBOOT_FILE_VER "bin" 
                elif [ $num = 3 ]; then
                        check_binary $DPL_KEY $DPL $DPL_FILE_VER "dtb"
                elif [ $num = 4 ]; then
                        check_binary $DPC_KEY $DPC $DPC_FILE_VER "dtb"
                elif [ $num = 5 ]; then
                        check_binary $MC_KEY $MC $MC_FILE_VER "itb"
                elif [ $num = 6 ]; then
                        check_binary $DTB_KEY $DTB $DTB_FILE_VER "dtb"
                elif [ $num = 7 ]; then
                        check_binary $UIMAGE_INIC_KEY $UIMAGE_INIC $UIMAGE_INIC_FILE_VER "bin"
                elif [ $num = 8 ]; then
                        check_binary $UIMAGE_ISSD_KEY $UIMAGE_ISSD $UIMAGE_ISSD_FILE_VER "bin"
                elif [ $num = 9 ]; then
                        check_binary $ROOTFS_KEY $ROOTFS $ROOTFS_FILE_VER "sqsh"
		elif [ $num = 10 ]; then
                        check_binary $FPGA_KEY $FPGA $FPGA_FILE_VER "rpd"
                fi
        done
        cd $ROOT_DIR
        checksum=$(md5sum -c ./checksums.md5 | grep OK | wc -l)

        if [ "$checksum" != "10" ]; then
        	print_error 6
                md5sum -c ./checksums.md5
                exit 1 
        fi
        cd ..
}

function update_pbl
{
	echo "updating $PBL_FILE_NAME ..."
   	flashcp -v $BINARY_PATH/$PBL_FILE_NAME $PBL_PARTITION
       
	if [ "$?" != "0" ]; then
    		print_error 5 $PBL_FILE_NAME
	else
       		display_success $PBL_FILE_NAME	
       	fi
}

function update_uboot
{
	echo "updating $UBOOT_FILE_NAME ..."
        echo "Erasing u-boot environment partition ..."
        flash_erase $UBOOT_ENV_PARTITION 0 0
        flashcp -v $BINARY_PATH/$UBOOT_FILE_NAME $UBOOT_PARTITION

        if [ "$?" != "0" ]; then
                print_error 5 $UBOOT_FILE_NAME
        else
                display_success $UBOOT_FILE_NAME
        fi
}

function update_dpl
{
        echo "updating $DPL_FILE_NAME ..."
        flashcp -v $BINARY_PATH/$DPL_FILE_NAME $DPL_PARTITION

        if [ "$?" != "0" ]; then
                print_error 5 $DPL_FILE_NAME
        else
                display_success $DPL_FILE_NAME
        fi
}

function update_dpc
{
        echo "updating $DPC_FILE_NAME ..."
        flashcp -v $BINARY_PATH/$DPC_FILE_NAME $DPC_PARTITION

        if [ "$?" != "0" ]; then
                print_error 5 $DPC_FILE_NAME
        else
                display_success $DPC_FILE_NAME
        fi
}

function update_mc
{
        echo "updating $MC_FILE_NAME ..."
        flashcp -v $BINARY_PATH/$MC_FILE_NAME $MC_PARTITION

        if [ "$?" != "0" ]; then
                print_error 5 $MC_FILE_NAME
        else
                display_success $MC_FILE_NAME
        fi
}

function update_dtb
{
        echo "updating $DTB_FILE_NAME ..."
        flashcp -v $BINARY_PATH/$DTB_FILE_NAME $DTB_PARTITION

        if [ "$?" != "0" ]; then
                print_error 5 $DTB_FILE_NAME
        else
                display_success $DTB_FILE_NAME
        fi
}

function update_uimage_inic
{
        echo "updating $UIMAGE_INIC_FILE_NAME ..."
        flashcp -v $BINARY_PATH/$UIMAGE_INIC_FILE_NAME $UIMAGE_INIC_PARTITION

        if [ "$?" != "0" ]; then
                print_error 5 $UIMAGE_INIC_FILE_NAME
        else
                display_success $UIMAGE_INIC_FILE_NAME
        fi
}

function update_uimage_issd
{
        echo "updating $UIMAGE_ISSD_FILE_NAME ..."
        flashcp -v $BINARY_PATH/$UIMAGE_ISSD_FILE_NAME $UIMAGE_ISSD_PARTITION
        
        if [ "$?" != "0" ]; then
                print_error 5 $UIMAGE_ISSD_FILE_NAME
        else
                display_success $UIMAGE_ISSD_FILE_NAME
        fi
}

function update_rootfs
{
        echo "updating $ROOTFS_FILE_NAME ..."
        flashcp -v $BINARY_PATH/$ROOTFS_FILE_NAME $ROOTFS_PARTITION
        
        if [ "$?" != "0" ]; then
                print_error 5 $ROOTFS_FILE_NAME
        else
                display_success $ROOTFS_FILE_NAME
        fi
}

function update_fpga
{
        echo "updating $FPGA_FILE_NAME ..."
      
        flash_fpga $BINARY_PATH/$FPGA_FILE_NAME

        if [ "$?" != "0" ]; then
                print_error 5 $FPGA_FILE_NAME
        else
                display_success $FPGA_FILE_NAME
   		display_shutdown
        fi
}

function read_fpga_image_version
{
	echo $(fpga_version)
}

function read_current_pbl_image_version
{
	$(hexdump $PBL_PARTITION | head -n 3 > tmp)
	pbi_checksum=$(sed -n 3p tmp | cut -c34-37)
	offset=$((0x$pbi_checksum - 0xc1))
	offset=$(($offset / 0x4))
	offset=$(($offset + 0xbc))
	$(split $PBL_PARTITION -b $offset)
	$(hexdump -C xab | head -n 1 > tmp)
	c5=$(cat tmp | cut -c62-69)
	$(rm -rf x* tmp)
        echo $c5
}

function read_version_of_image
{
        file_name=$1

	$(hexdump -C $file_name | head -n 1 > tmp)
	c5=$(cat tmp | cut -c62-69)
        rm -rf tmp
        echo $c5
}

function get_kernel_version
{
	var=$(cat /proc/kernel_ver)
        ret=${var#*:}
        echo $ret
}

function get_uboot_version
{
	var=$(cat /proc/cmdline)
	var1=${var#*ubootver=}
        echo $var1 | cut -c1-8
}

function get_rootfs_version
{
        c5=$(cat /etc/version | head -n 1 | cut -c1-8)
	echo $c5
}

function update_binary
{
	file_name=$1
        file_ver=$2
        line=$3

 	case $line in
             1)  retval=$(read_current_pbl_image_version)
                 if [ ${file_ver} = ${retval} ]; then
                 display_binary_exist_msg $file_name
        	 else
 		 update_pbl
                 fi 
                 ;;
             2)  retval=$(get_uboot_version)
                 if [ ${file_ver} = ${retval} ]; then
                 display_binary_exist_msg $file_name
                 else
                 update_uboot
                 fi
                 ;;
  	     3)  retval=$(read_version_of_image $DPL_PARTITION)
                 if [ ${file_ver} = ${retval} ]; then
                 display_binary_exist_msg $file_name
                 else
                 update_dpl
                 fi
                 ;;
             4) retval=$(read_version_of_image $DPC_PARTITION)
                 if [ ${file_ver} = ${retval} ]; then
                 display_binary_exist_msg $file_name
                 else
                 update_dpc
                 fi
		 ;;
             5)  retval=$(read_version_of_image $MC_PARTITION)
                 if [ ${file_ver} = ${retval} ]; then
                 display_binary_exist_msg $file_name
                 else
                 update_mc
                 fi
  		 ;;
             6)  update_dtb ;;

             7)  retval=$(get_kernel_version)
                 if [ ${file_ver} = ${retval} ]; then
                 display_binary_exist_msg $file_name
                 else
                 update_uimage_inic
                 fi
                 ;;
             8)  retval=$(get_kernel_version)
                 if [ ${file_ver} = ${retval} ]; then
                 display_binary_exist_msg $file_name
                 else
                 update_uimage_issd
                 fi
                 ;;
             9)  retval=$(get_rootfs_version)
                 if [ ${file_ver} = ${retval} ]; then
                 display_binary_exist_msg $file_name
                 else
                 update_rootfs
                 fi
                 ;;
             10) retval=$(read_fpga_image_version)
                 if [ ${file_ver} = ${retval} ]; then
                 display_binary_exist_msg $file_name
                 else
                 update_fpga
                 fi
                 ;;
             *) echo "Error: Invalid image number"
        esac
}

function update_images
{      
	for num in {1..9}
        do
        	if [ $num = 1 ]; then
        		update_binary $PBL_FILE_NAME $PBL_FILE_VER $num
        	elif [ $num = 2 ]; then
        		update_binary $UBOOT_FILE_NAME $UBOOT_FILE_VER $num
        	elif [ $num = 3 ]; then
        		update_binary $DPL_FILE_NAME $DPL_FILE_VER $num
        	elif [ $num = 4 ]; then
        		update_binary $DPC_FILE_NAME $DPC_FILE_VER $num
        	elif [ $num = 5 ]; then
        		update_binary $MC_FILE_NAME $MC_FILE_VER $num
        	elif [ $num = 6 ]; then
        		update_binary $DTB_FILE_NAME $DTB_FILE_VER $num
                elif [ $num = 7 ]; then
                        update_binary $UIMAGE_INIC_FILE_NAME $UIMAGE_INIC_FILE_VER $num
                elif [ $num = 8 ]; then
                        update_binary $UIMAGE_ISSD_FILE_NAME $UIMAGE_ISSD_FILE_VER $num
                elif [ $num = 9 ]; then
                        update_binary $ROOTFS_FILE_NAME $ROOTFS_FILE_VER $num
        	fi
        done
}

function update
{
	case $USER_OPT in

        	"all")
	       		disply
               		update_images
                	;;
		"factory_default")
			disply
			update_pbl
			update_uboot
			update_dpl
			update_dpc
			update_mc
			update_dtb
			update_uimage_inic
			update_uimage_issd
			update_rootfs
			;;
        	"force_all")
			disply
                	update_pbl
                	update_uboot
                	update_dpl
			update_dpc
			update_mc
			update_dtb
			update_uimage_inic
                	update_uimage_issd
                	update_rootfs
                	;;
        	"pbl")
			disply
                	update_binary $PBL_FILE_NAME $PBL_FILE_VER 1
			;;
        	"u-boot")
			disply
                	update_binary $UBOOT_FILE_NAME $UBOOT_FILE_VER 2
                	;;
        	"dpl")
			disply
                	update_binary $DPL_FILE_NAME $DPL_FILE_VER 3
                	;;
        	"dpc")
			disply
                	update_binary $DPC_FILE_NAME $DPC_FILE_VER 4
                	;;
        	"mc")
			disply
                	update_binary $MC_FILE_NAME $MC_FILE_VER 5
                	;;
        	"dtb")	
			disply
                	update_binary $DTB_FILE_NAME $DTB_FILE_VER 6
                	;;
        	"uimage_inic")
			disply
                	update_binary $UIMAGE_INIC_FILE_NAME $UIMAGE_INIC_FILE_VER 7
                	;;
        	"uimage_issd")
			disply
                	update_binary $UIMAGE_ISSD_FILE_NAME $UIMAGE_ISSD_FILE_VER 8
                	;;
        	"rootfs")
			disply
                	update_binary $ROOTFS_FILE_NAME $ROOTFS_FILE_VER 9
                	;;
        	"fpga")
			display_fpga
                	update_binary $FPGA_FILE_NAME $FPGA_FILE_VER 10
                	;;
        	"force_fpga")
			display_fpga
                	update_fpga
                	;;
         	*)
                	echo "Invalid command"
                	exit 1
                	;;
	esac
}

function update_raw
{
        option=$1
        file=$2
        file_name=${file##*/};

	case $option in

        	"pbl")
			disply
                	echo "updating $file_name ..."
                        flashcp -v $file $PBL_PARTITION
                        if [ "$?" != "0" ]; then
                        	print_error 5 $file_name
        		else
                		display_success $file_name
        		fi
			;;
        	"u-boot")
			disply
                	echo "updating $file_name ..."
        		echo "Erasing u-boot environment partition ..."
        		flash_erase $UBOOT_ENV_PARTITION 0 0
        		flashcp -v $file $UBOOT_PARTITION

        		if [ "$?" != "0" ]; then
                		print_error 5 $file_name
        		else
                		display_success $file_name
        		fi
                	;;
        	"dpl")
			disply
                	echo "updating $file_name ..."
                        flashcp -v $file $DPL_PARTITION
                        if [ "$?" != "0" ]; then
                        	print_error 5 $file_name
        		else
                		display_success $file_name
        		fi
                	;;
        	"dpc")
			disply
                	echo "updating $file_name ..."
                        flashcp -v $file $DPC_PARTITION
                        if [ "$?" != "0" ]; then
                        	print_error 5 $file_name
        		else
                		display_success $file_name
        		fi
                	;;
        	"mc")
			disply
                	echo "updating $file_name ..."
                        flashcp -v $file $MC_PARTITION
                        if [ "$?" != "0" ]; then
                        	print_error 5 $file_name
        		else
                		display_success $file_name
        		fi
                	;;
        	"dtb")	
			disply
                	echo "updating $file_name ..."
                        flashcp -v $file $DTB_PARTITION
                        if [ "$?" != "0" ]; then
                        	print_error 5 $file_name
        		else
                		display_success $file_name
        		fi
                	;;
        	"uimage_inic")
			disply
                	echo "updating $file_name ..."
                        flashcp -v $file $UIMAGE_INIC_PARTITION
                        if [ "$?" != "0" ]; then
                        	print_error 5 $file_name
        		else
                		display_success $file_name
        		fi
                	;;
        	"uimage_issd")
			disply
                        echo "updating $file_name ..."
                        flashcp -v $file $UIMAGE_ISSD_PARTITION
                        if [ "$?" != "0" ]; then
                        	print_error 5 $file_name
        		else
                		display_success $file_name
        		fi
                	;;
        	"rootfs")
			disply
                	echo "updating $file_name ..."
                        flashcp -v $file $ROOTFS_PARTITION
                        if [ "$?" != "0" ]; then
                        	print_error 5 $file_name
        		else
                		display_success $file_name
        		fi
                	;;
                "fpga")
                        display_fpga
        		echo "updating $file_name ..."
        		flash_fpga $file

                        if [ "$?" != "0" ]; then
                		print_error 5 $file_name
        		else
                		display_success $file_name
   				display_shutdown
        		fi
			;;
         	*)
                	echo "Invalid argument"
                	exit 1
                	;;
	esac
}

function check_file_or_tar
{
        if [ -d $ROOT_DIR ]; then
                init
        	validate_binaries
        	update
        elif [ -f "$ROOT_DIR" ]; then
        	echo "WARNING: updating binary without version and validation check"
       		update_raw $USER_OPT $ROOT_DIR
        else
        	echo "ERROR: Invalid argument"
        fi
}

check_file_or_tar
