DESCRIPTION = "A python package that works to provide a nice set of testing utilities for the kazoo library."
HOMEPAGE = "https://github.com/yahoo/Zake"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://PKG-INFO;md5=7143f0ea4f9516655b2b2a360e8e71f7"

PR = "r0"

SRCNAME = "zake"
SRC_URI = "http://pypi.python.org/packages/source/z/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "51a9570a1e93d7eee602a240b0f5ba21"
SRC_URI[sha256sum] = "b6ed2f9c957225bf371a76dccf25a4300f33f61aa2fd560ab89673200e52abfc"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

# DEPENDS_default: python-pip

DEPENDS += " \
        python-pip \
        "

# RDEPENDS_default: 
RDEPENDS_${PN} += " \
        python-kazoo \
        "
