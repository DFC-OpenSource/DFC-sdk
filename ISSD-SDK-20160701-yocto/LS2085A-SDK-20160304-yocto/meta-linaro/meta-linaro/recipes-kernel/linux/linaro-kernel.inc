DESCRIPTION = "Linaro Kernel common bits"
SECTION = "kernel"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=d7810fab7487fb0aad327b76f1be7cd7"

inherit kernel siteinfo
require recipes-kernel/linux/linux-dtb.inc

SRC_URI = " \
           git://git.linaro.org/git/ci/ubuntu-kernel-ci.git;protocol=http;branch=master;name=ubuntu-ci;destsuffix=ubuntu-ci \
"
S = "${WORKDIR}/git"

SRCREV_ubuntu-ci = "33beed2b73c3de0556317317034569e909e90f1d"
SRCREV_FORMAT = "kernel"
