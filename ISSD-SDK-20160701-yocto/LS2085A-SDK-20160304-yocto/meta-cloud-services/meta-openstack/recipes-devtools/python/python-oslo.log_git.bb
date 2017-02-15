DESCRIPTION = "Oslo Log Library"
HOMEPAGE = "https://launchpad.net/oslo"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=34400b68072d710fecd0a2940a0d1658"

SRCNAME = "oslo.log"
SRC_URI = "git://github.com/openstack/${SRCNAME}.git;branch=stable/kilo"

PV = "1.0.0"
SRCREV = "4f01e7e540e5cc6b26d32f2bc56dccd3fcd21920"
S = "${WORKDIR}/git"

inherit setuptools

DEPENDS += " \
        python-pip \
        python-babel \
        python-pbr \
        "

# RDEPENDS_default: 
RDEPENDS_${PN} += " \
        bash \
        python-babel \
        python-debtcollector \
        python-pbr \
        python-iso8601 \
        python-oslo.config \
        python-oslo.context \
        python-oslo.i18n \
        python-oslo.utils \
        python-oslo.serialization\
        python-six \
        "
