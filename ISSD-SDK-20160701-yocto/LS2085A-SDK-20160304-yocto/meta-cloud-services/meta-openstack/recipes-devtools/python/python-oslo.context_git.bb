DESCRIPTION = "Oslo Context Library"
HOMEPAGE = "https://launchpad.net/oslo"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=1dece7821bf3fd70fe1309eaa37d52a2"

SRCNAME = "oslo.context"
SRC_URI = "git://github.com/openstack/${SRCNAME}.git;branch=stable/kilo"

PV = "0.2.0"
SRCREV = "6dde0a3ee69c44a0fd836fdd520f6efb1cb34229"
S = "${WORKDIR}/git"

inherit setuptools

DEPENDS += " \
        python-pip \
        python-babel \
        python-pbr \
        "

# RDEPENDS_default: 
RDEPENDS_${PN} += " \
        python-babel \
        python-pbr \
        bash \
        "
