sudo mkdir -p /usr/share/bdbm_drv
sudo touch /usr/share/bdbm_drv/ftl.dat
sudo touch /usr/share/bdbm_drv/dm.dat

sudo insmod ../bdbm_drv.ko
sudo ./bdbm_format /dev/blueDBM
sudo mkfs -t ext4 -b 4096 /dev/blueDBM
sudo mount \-t ext4 \-o discard /dev/blueDBM /media/blueDBM
