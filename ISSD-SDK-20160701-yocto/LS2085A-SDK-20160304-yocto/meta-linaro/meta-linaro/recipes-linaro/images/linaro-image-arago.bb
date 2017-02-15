SUMMARY = "Arago based image for testing"
DESCRIPTION = "Image capable of booting and testing device."
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

require linaro-image-common.inc

# List of packages to install
IMAGE_INSTALL += "\
    bc \
    bonnie++ \
    bridge-utils \
    evtest \
    hdparm \
    iozone3 \
    iperf \
    lmbench \
    memtester \
    rt-tests \
    "
