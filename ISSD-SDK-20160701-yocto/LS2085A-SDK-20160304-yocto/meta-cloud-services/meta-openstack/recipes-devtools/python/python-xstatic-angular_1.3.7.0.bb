DESCRIPTION = "Angular JavaScript library packaged for setuptools"
HOMEPAGE = "https://pypi.python.org/pypi/XStatic-Angular"
SECTION = "devel/python"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://PKG-INFO;md5=fef10afacf31c10c1fb9ce04e3763269"

PR = "r0"

SRCNAME = "XStatic-Angular"
SRC_URI = "http://pypi.python.org/packages/source/X/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "fa26884ab0b2acfa09e372aedd7e7b04"
SRC_URI[sha256sum] = "7cad216b226399cbe8d2909ed5b9f28d724907b9c9e1e078e6e25d320a3d5dd7"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

# DEPENDS_default: python-pip

DEPENDS += " \
        python-pip \
        "

# RDEPENDS_default: 
RDEPENDS_${PN} += " \
        "
