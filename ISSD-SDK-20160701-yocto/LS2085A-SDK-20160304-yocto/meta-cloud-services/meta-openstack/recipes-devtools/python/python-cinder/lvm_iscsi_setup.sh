#Load volume group

[[ -f /etc/cinder/volumes-backing ]] || truncate -s %CINDER_LVM_VOLUME_BACKING_FILE_SIZE% /etc/cinder/volumes-backing

DEV=`losetup -j /etc/cinder/volumes-backing | cut -d ":" -f 1 | head -1`
if [ -z $DEV ]; then
    DEV=`losetup -f --show /etc/cinder/volumes-backing`
fi
if ! vgs cinder-volumes &> /dev/null ; then
	    vgcreate cinder-volumes $DEV
fi

vgchange -ay cinder-volumes
