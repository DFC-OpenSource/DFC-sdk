DESCRIPTION = "Openstack controller node image"
LICENSE = "MIT"

OPENSTACK_CONTROLLER_EXTRA_INSTALL ?= ""

IMAGE_INSTALL = "\
    ${CORE_IMAGE_BASE_INSTALL} \
    ${ROOTFS_PKGMANAGE_BOOTSTRAP} \
    packagegroup-core-full-cmdline \
    packagegroup-cloud-controller \
    packagegroup-cloud-network \
    packagegroup-cloud-debug \
    packagegroup-cloud-extras \
    ${@base_contains('OPENSTACK_EXTRA_FEATURES', 'benchmarking', 'packagegroup-cloud-benchmarking', '', d)} \
    ${OPENSTACK_CONTROLLER_EXTRA_INSTALL} \
    "

IMAGE_FEATURES += " ssh-server-openssh package-management"
POST_KEYSTONE_SETUP_COMMAND = "/etc/keystone/hybrid-backend-setup"

inherit core-image
inherit openstack-base
inherit identity
inherit monitor

# Ensure extra space for guest images, and rabbit MQ has a hard coded
# check for 2G of free space, so we use 5G as a starting point.
IMAGE_ROOTFS_EXTRA_SPACE_append += "+ 5000000"

