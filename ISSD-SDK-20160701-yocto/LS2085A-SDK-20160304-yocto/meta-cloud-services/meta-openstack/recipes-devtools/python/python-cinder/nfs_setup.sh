#! /bin/bash

is_default=%IS_DEFAULT%
if [ $# -eq 1 ]; then
    is_default=$1
fi

# Default setup
if [ "$is_default" == "1" ]; then
    mkdir /etc/cinder/nfs_volumes
    echo "/etc/cinder/nfs_volumes *(rw,nohide,insecure,no_subtree_check,async,no_root_squash)" >> /etc/exports
fi
