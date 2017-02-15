DESCRIPTION = "Openstack all-in-one node image"
LICENSE = "MIT"

OPENSTACK_CONTROLLER_EXTRA_INSTALL ?= ""
OPENSTACK_COMPUTE_EXTRA_INSTALL ?= ""
OPENSTACK_AIO_EXTRA_INSTALL ?= ""

IMAGE_INSTALL = " \
    ${ROOTFS_PKGMANAGE_BOOTSTRAP} \
    ${CORE_IMAGE_BASE_INSTALL} \
    packagegroup-core-full-cmdline \
    packagegroup-cloud-compute \
    packagegroup-cloud-controller \
    packagegroup-cloud-network \
    packagegroup-cloud-debug \
    packagegroup-cloud-extras \
    ${OPENSTACK_CONTROLLER_EXTRA_INSTALL} \
    ${OPENSTACK_COMPUTE_EXTRA_INSTALL} \
    ${OPENSTACK_AIO_EXTRA_INSTALL} \
    ${@base_contains('OPENSTACK_EXTRA_FEATURES', 'benchmarking', 'task-cloud-benchmarking', '', d)} \
    "

IMAGE_FEATURES += "ssh-server-openssh"

inherit core-image
inherit openstack-base
inherit identity
inherit monitor

# check for 5G of free space, so we use 5G as a starting point.
IMAGE_ROOTFS_EXTRA_SPACE_append += "+ 5000000"

POST_KEYSTONE_SETUP_COMMAND = "/etc/keystone/hybrid-backend-setup"

