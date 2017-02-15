DESCRIPTION = "Openstack network node image"
LICENSE = "MIT"

IMAGE_INSTALL = "\
    ${CORE_IMAGE_BASE_INSTALL} \
    ${ROOTFS_PKGMANAGE_BOOTSTRAP} \
    packagegroup-core-full-cmdline \
    packagegroup-cloud-network \
    "

IMAGE_FEATURES += " ssh-server-openssh"

inherit core-image

# Ensure extra space for guest images
IMAGE_ROOTFS_EXTRA_SPACE = "2000000"
