#
# Copyright (C) 2013 Freescale Semiconductor Inc.
#
# This image is designed to run from flash. It only includes essetional
# packages to deploy other big images to large physical media, such as
# usb stick, hard drive.

require images/fsl-image-minimal.bb

IMAGE_INSTALL += " \
    packagegroup-core-ssh-openssh \
    bash \
    dosfstools \
    e2fsprogs \
    e2fsprogs-e2fsck \
    e2fsprogs-mke2fs \
    e2fsprogs-tune2fs \
    kmod \
    mtd-utils \
    mtd-utils-jffs2 \
    net-tools \
    sysfsutils \
    sysklogd \
    sysstat \
    util-linux \
"
