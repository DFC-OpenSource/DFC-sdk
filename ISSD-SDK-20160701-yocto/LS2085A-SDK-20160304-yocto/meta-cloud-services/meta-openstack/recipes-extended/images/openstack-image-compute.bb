DESCRIPTION = "Openstack compute node image"
LICENSE = "MIT"

OPENSTACK_COMPUTE_EXTRA_INSTALL ?= ""

IMAGE_INSTALL = " \
    packagegroup-core-boot \
    ${ROOTFS_PKGMANAGE_BOOTSTRAP} \
    packagegroup-cloud-compute \
    packagegroup-cloud-debug \
    packagegroup-cloud-extras \
    ${OPENSTACK_COMPUTE_EXTRA_INSTALL} \
    "

IMAGE_FEATURES += "ssh-server-openssh"

inherit core-image
inherit openstack-base
inherit monitor

# Ensure extra space for guest images, and rabbit MQ has a hard coded
# check for 2G of free space, so we use 3G as a starting point.
IMAGE_ROOTFS_EXTRA_SPACE_append += "+ 3000000"

# ROOTFS_POSTPROCESS_COMMAND += "remove_packaging_data_files ; "
