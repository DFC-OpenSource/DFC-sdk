DESCRIPTION = "%DESCRIPTION%"
HOMEPAGE = "%URL%"
SECTION = "devel/python"
LICENSE = "Apache-2"
LIC_FILES_CHKSUM = "file://PKG-INFO;md5=21ce610a6aa67eed6d6867db316597ee"

PR = "r0"

SRCNAME = "XStatic-Bootstrap-SCSS"
SRC_URI = "http://pypi.python.org/packages/source/X/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "903d7de8aef6bee93d976b42d35edf7b"
SRC_URI[sha256sum] = "7e9858737e2e6aac921ec7a3fca627e522901c3061611e154ebc0b8a892c7018"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

# DEPENDS_default: python-pip

DEPENDS += " \
        python-pip \
        "

# RDEPENDS_default: 
RDEPENDS_${PN} += " \
        "
