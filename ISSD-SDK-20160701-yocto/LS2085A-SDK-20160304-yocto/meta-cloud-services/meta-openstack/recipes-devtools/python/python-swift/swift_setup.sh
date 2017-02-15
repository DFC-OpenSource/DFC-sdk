#!/bin/bash

#set -x

SWIFT_RING_BUILDER=/usr/bin/swift-ring-builder


for_each_dev_in_cluster ()
{
    local cmd=$1

    dev_id=0
    while read line; do
        line=`echo $line | sed -e 's/^ *//g' -e 's/ *$//g'`
        [ -z "$line" ] && continue

        dev=`echo $line | cut -d ',' -f 1 | sed -e 's/^ *//g' -e 's/ *$//g'`
        dev_format=`echo $line | cut -d ',' -f 2 | sed -e 's/^ *//g' -e 's/ *$//g'`
        zone=`echo $line | cut -d ',' -f 3 | sed -e 's/^ *//g' -e 's/ *$//g'`
        ring_names=`echo $line | cut -d ',' -f 4 | sed -e 's/^ *//g' -e 's/ *$//g'`
        weight=`echo $line | cut -d ',' -f 5 | sed -e 's/^ *//g' -e 's/ *$//g'`

        [[ "${dev:0:1}" == "#" ]] && continue
        if [ ! -z $dev ] && [ ! -e $dev ]; then
            continue
        fi
        dev_id=$((dev_id+1))
        $cmd "$dev" "$dev_format" "$dev_id" "$zone" "$ring_names" "$weight" $2 $3 $4
    done < $cluster_conf
}


mount_dev ()
{
    local dev=$1
    local dev_id=$3
    local zone=$4

    mkdir -p $swift_home/node/z${zone}d${dev_id}
    [ -z "$dev" ] && return 1
    if [ -b $dev ]; then
        mount $dev $swift_home/node/z${zone}d${dev_id}
    else
        loopdev=`losetup --show -f $dev`
        mount $loopdev $swift_home/node/z${zone}d${dev_id}
    fi
}


unmount_dev ()
{
    local dev=$1

    [ -z "$dev" ] && return 1
    if [ -b $dev ]; then
        umount $dev
    else
        loopdev=`losetup -j $dev | cut -d ":" -f 1 | tail -1`
        if [ ! -z $loopdev ]; then
            umount $loopdev
            losetup -d $loopdev
        fi
    fi
}


format_dev ()
{
    local dev=$1
    local dev_format=$2

    [ -z "$dev" ] && return 1
    if [ -b $dev ]; then
        mkfs.${dev_format} $dev
    else
        loopdev=`losetup --show -f $dev`
        mkfs.${dev_format} $loopdev
        sleep 2
        losetup -d $loopdev
    fi
}


add_dev ()
{
    local dev_id=$3
    local zone=$4
    local ring_names=$5
    local weight=$6

    account_port=6002
    container_port=6001
    object_port=6000

    for ring in $ring_names; do
        eval builder_port=\$${ring}_port
        [ -z $builder_port ] && continue
        $SWIFT_RING_BUILDER $swift_home/${ring}.builder add -r 1 -z $zone -i $host_ip -p $builder_port -d z${zone}d${dev_id} -w $weight
    done
}


cluster_create_rings ()
{
    if [ ! -f $SWIFT_RING_BUILDER ]; then
        echo "ERROR: Swift packages are not installed on this machine"
        return 1
    fi

    if [ -e $swift_home/account.builder -o -e $swift_home/container.builder -o -e $swift_home/object.builder ]; then
        echo "ERROR: Swift cluster has been created"
        return 1
    fi

    sed "s/^swift_hash_path_suffix =.*/swift_hash_path_suffix = `uuidgen`/" -i $swift_home/swift.conf

    # Create 3 ring builders: each ring has 2^12 partition
    $SWIFT_RING_BUILDER $swift_home/account.builder create 12 $cluster_replica 1
    $SWIFT_RING_BUILDER $swift_home/container.builder create 12 $cluster_replica 1
    $SWIFT_RING_BUILDER $swift_home/object.builder create 12 $cluster_replica 1
}


cluster_add_devs ()
{
    for_each_dev_in_cluster add_dev

    $SWIFT_RING_BUILDER $swift_home/account.builder rebalance
    $SWIFT_RING_BUILDER $swift_home/container.builder rebalance
    $SWIFT_RING_BUILDER $swift_home/object.builder rebalance
}


cluster_delete_all ()
{
    for_each_dev_in_cluster unmount_dev
    rm -rf $swift_home/*.ring.gz
    rm -rf $swift_home/*.builder
    rm -rf $swift_home/node
}


help ()
{
    cat << EOF
usage:  swift_setup.sh [-hcirk] <command>

 -h : View this help
 -c : Swift cluster config file, default is /etc/swift/cluster.conf
 -i : IP address of this machine, default is 127.0.0.1
 -r : The number of Swift replications
 -k : Location where Swift builder and ring files stay, default is /etc/swift

<command> can be one of the followings:

 createrings : Create Swift account, container, and object builder
       clean : Delete all Swift build and ring files
   mountdevs : Mount all devices specified in cluster config file
 unmountdevs : Un-mount all devices specified in cluster config file
  formatdevs : Format all devices specified in cluster config file
     adddevs : Add all devices specified in cluster config file into
               Swift rings

swift_setup.sh script helps to create or expand Swift cluster with
storage devices in this machine.

The Swift cluster config file (default is /etc/swift/cluster.conf) describes
how storage devices in this machine can be added into Swift cluster.

Each line of cluster config file specifies how one storage device can be
added into Swift cluster with the following format:

    <dev file>, <dev format>, <zone>, <ring names>, <weight>

 <dev file>: full path to a block device or full path to regular
             file which is used for backing up a loopback device.
             If <dev file> is empty string then this device is
             backed up by a directory in local filesystem.
 <dev format>: the disk format type, e.g. ext4, xfs...
 <zone>: the zone id which this block device belongs to
 <ring names>: a list (separated by space) ring name ("account", "container",
               or "object") which this device is added to
 <weight>: weight of this device in Swift zone


Example 1: To add storage devices in this machine into an existing
Swift Cluster:

    $ /etc/init.d/swift stop
    $ Sync the Swift builders and rings files into this machine
    $ Modify /etc/cluster.conf to specify what storage devices
      are added into cluster
    $ swift_setup.sh formatdevs
    $ swift_setup.sh mountdevs
    $ swift_setup.sh -i <IP of this machine> adddevs
    $ swift_setup.sh unmountdevs
    $ /etc/init.d/swift start

Example 2: To create an new Swift cluster:

    $ /etc/init.d/swift stop
    $ Modify /etc/cluster.conf to reflect the desired cluster
    $ swift_setup.sh clean
    $ swift_setup.sh createrings
    $ swift_setup.sh formatdevs
    $ swift_setup.sh mountdevs
    $ swift_setup.sh -i <IP of this machine> adddevs
    $ swift_setup.sh unmountdevs
    $ /etc/init.d/swift start
EOF
}


cluster_conf="/etc/swift/cluster.conf"
swift_home="/etc/swift"
host_ip="127.0.0.1"
cluster_replica="2"

while getopts "?hc:r:i:k:" opt; do
    case $opt in
        c)
            cluster_conf=$OPTARG
            ;;
        r)
            cluster_replica=$OPTARG
            ;;
        i)
            host_ip=$OPTARG
            ;;
        k)
            swift_home=$OPTARG
            ;;
        ?|h)
            help
            exit 0
            ;;
    esac
done

# Move aside the optional arguments
shift $(( $OPTIND - 1 ))


case "$1" in
    createrings)
        cluster_create_rings
        ;;
    clean)
        for_each_dev_in_cluster unmount_dev
        cluster_delete_all
        ;;
    mountdevs)
        for_each_dev_in_cluster mount_dev
        ;;
    unmountdevs)
        for_each_dev_in_cluster unmount_dev
        ;;
    formatdevs)
        for_each_dev_in_cluster format_dev
        ;;
    adddevs)
        cluster_add_devs
        ;;
    *)
        help
        exit 0
        ;;
esac

exit 0
