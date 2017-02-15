var1=$(cat /proc/cmdline)
var2=${var1##*bootimg=}
echo $var2 > /run/temp1
var3=$(cat /run/temp1 | cut -d' ' -f1)
echo bootimg $var3
if [ "$var3" = "issd-sa" ]
then	
	echo 0 > /proc/sys/kernel/printk
	sysctl -w vm.dirty_ratio=5
	sysctl -w vm.dirty_background_ratio=1
	echo "Started Standalone iSSD controller"
	cp -a /usr/sbin/issd_nvme_stda /run/.
	/run/issd_nvme_stda &
	sleep 2
	insmod /lib/modules/4.1.8/kernel/drivers/block/nvme.ko
fi
if [ "$var3" = "issd" ]
then	
	echo 0 > /proc/sys/kernel/printk
	insmod /lib/modules/4.1.8/kernel/drivers/uio/uio_pci_generic.ko
	sysctl -w vm.dirty_ratio=5
	sysctl -w vm.dirty_background_ratio=1
	echo "Started iSSD controller"
	cp -a /usr/sbin/issd_nvme /run/.
	/run/issd_nvme &
fi
rm /run/temp1

# For making mnt directory as a log partition with  jffs2 file system.

#mount -t jffs2 /dev/mtdblock10 /mnt
