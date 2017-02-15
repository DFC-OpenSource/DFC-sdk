# Filename   :   upgrade_images.sh
#
# Author     :   
#
# Description:   Script for flashing binary into iNIC card.

USER_OPT=`echo $1 | tr "[:upper:]" "[:lower]"`
TAR_FILE_PATH=$2

MOUNT_PATH=/run
TAR_FILE_NAME="${TAR_FILE_PATH##*/}"
TAR_FILE_DIR=$MOUNT_PATH/${TAR_FILE_NAME%.tar*};
TAR_FILE_LOC=${TAR_FILE_PATH%/*};

function extract_tarball
{
        if [ -d $TAR_FILE_DIR ]; then
        	echo "Removing $TAR_FILE_DIR"
        	rm -rf $TAR_FILE_DIR
        fi

	if [ -f "$TAR_FILE_PATH" ]; then
                cd $MOUNT_PATH
		tar -xvzf $TAR_FILE_PATH
                cd $TAR_FILE_DIR
                chmod 777 * ./images/*
   		if [ $? != '0' ]; then
   			echo "ERROR: extracting tar ball failed"
                	exit 1
   		fi
	else
		echo "ERROR: tar ball does not exist"
        	exit 1
	fi
}

function remove_tar
{

  if [ -d $TAR_FILE_DIR ]; then
  	echo "Removing $TAR_FILE_DIR"
        rm -rf $TAR_FILE_DIR
  fi
}

function upgrade_binary
{
	if [ -d "$TAR_FILE_DIR" ]; then
		if [ ! -f "$TAR_FILE_DIR/flash_images.sh" ]; then
			echo "ERROR: flash_images.sh does not exist"
              		exit 1
		else
			flash_images.sh $TAR_FILE_DIR $USER_OPT
                        remove_tar
        	fi
	else
		echo "ERROR: root directory does not exist"
        	exit 1
	fi
        
}
 
function help()
{
        echo    ""
        echo    "USAGE:"
        echo    " "
        echo 	"./upgrade.sh <argument-1> <argument-2>"
        echo    " "
        echo 	"Argument-1:"
        echo    "----------"
        echo 	"help         	  - displays this help text"
        echo    "all              - check the current version of all images "
        echo 	"                    and flash if it is required"
        echo    "factory_default  - flash all images without version check"
        echo    "force_all        - flash all images without version check"
	echo    "dpl          - check dpl current version and flash if required"
	echo    "dpc          - check dpc current version and flash if required"
	echo    "mc           - check mc current version and flash if required"
	echo    "dtb          - check dtb current version and flash if required"
	echo    "uimage_inic  - check uimage_inic current version and flash if required"
	echo    "uimage_issd  - check uimage_issd current version and flash if required"
	echo    "rootfs       - check rootfs current version and flash if required"
        echo    "fpga         - check storage card fpga current version and flash if required"
        echo    "force_fpga   - flash fpga image without version check"
        echo    ""
        echo    "Argument-2:"
        echo    "----------"
        echo    "path         - tar file full path ex: /run/<mount_dir>/issd-images_01.00.00.tar.gz"
        echo    "                                             <or>                                   "
        echo    "               full path of the binary which you would like to flash"
        echo    "               ex: /run/<mount_dir>/u-boot_01.01.00.bin"
        echo    ""
        echo    "Note:- all, force_all and force_fpga options can not be used if you are passing only"
        echo    "       binary file path as argument-2"
	echo    ""
}

case $USER_OPT in

      "all" | "force_all" | "factory_default" | "dpl" | "dpc" | "mc" | "dtb" | "uimage_inic" | "uimage_issd" | "rootfs" | "fpga" | "force_fpga")

		 if [ ! -f "$TAR_FILE_PATH" ]; then
			echo "ERROR: Invalid file path"
                        help
			exit 1
		 fi
                 
		 if [ "${TAR_FILE_PATH##*.tar}" = ".gz" ]; then
                 	extract_tarball
                        upgrade_binary
                 else
			flash_images.sh $TAR_FILE_PATH $USER_OPT			
                 fi
		 ;;
        "help")
                 help
                ;;
        *)
                echo "Invalid command"
                help
                exit 1
                ;;
esac
