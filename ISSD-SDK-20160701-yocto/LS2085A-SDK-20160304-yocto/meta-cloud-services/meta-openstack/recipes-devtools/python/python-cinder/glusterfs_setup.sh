#! /bin/bash

is_default=%IS_DEFAULT%
if [ $# -eq 1 ]; then
    is_default=$1
fi

# Default setup
if [ "$is_default" == "1" ]; then
    mkdir /etc/cinder/glusterfs_volumes
    /etc/init.d/glusterd start
    gluster volume create glusterfs_volumes controller:/etc/cinder/glusterfs_volumes force
    gluster volume start glusterfs_volumes force
    /etc/init.d/glusterd stop
fi
