SUMMARY = "Extra packages that improve the usability of compute/control nodes"
PR = "r0"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COREBASE}/LICENSE;md5=3f40d7994397109285ec7b81fdeb3b58 \
                    file://${COREBASE}/meta/COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"

inherit packagegroup

RDEPENDS_${PN} = " \
    vim \
    ${@base_contains('DISTRO_FEATURES', 'x11', 'xterm', '', d)} \
    "

IMAGE_FEATURES += "package-management"


