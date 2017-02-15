DESCRIPTION = "tablib formatters for cliff"
HOMEPAGE = "https://github.com/dreamhost/cliff-tablib"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=3b83ef96387f14655fc854ddc3c6bd57"

SRC_URI = "git://github.com/dreamhost/cliff-tablib;branch=master"
# 1.1 release
SRCREV="a83bf47d7dfbb690dd53e189c532f0859898db04"
PV="1.1+git${SRCPV}"
S = "${WORKDIR}/git"

inherit setuptools

DEPENDS += " \
    python-pbr \
    "

RDEPENDS_${PN} += " \
    python-pbr \
    python-cliff \
    python-tablib \
    "
