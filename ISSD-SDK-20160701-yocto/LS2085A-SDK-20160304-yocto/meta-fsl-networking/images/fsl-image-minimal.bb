# Copyright(C) 2014-2015 Freescale
#
# The minimal rootfs with basic packages for boot up
#

include recipes-core/images/core-image-minimal.bb
require images/fsl-image-deploy.inc

inherit fsl-utils

IMAGE_INSTALL += " \
    echo-server \
    kernel-image \
    restool \
    setserial \
    kernel-modules \
    kmod \   
    pciutils \
    i2c-tools \
    memtool \
    mtd-utils \
    mtd-utils-jffs2 \
    bash \
    iperf \
    i2capp \
    merge-files \
    flashfpga \
    issd-nvme \
    fio \
    sysklogd \
    lio-utils \
    ofed \
    open-iscsi-arm64 \
    python-2.7-aarch64 \
    targetcli \
"

IMAGE_FSTYPES = "ext2.gz tar.gz"

ROOTFS_POSTPROCESS_COMMAND += "rootfs_add_sdk_version;"
