DESCRIPTION = "Google's IP address manipulation library"
HOMEPAGE = "http://code.google.com/p/ipaddr-py/"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://PKG-INFO;md5=d8b8c1199001505d7b77da5db34ba441"

PR = "r0"

SRCNAME = "ipaddr"
SRC_URI = "http://pypi.python.org/packages/source/i/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "f2c7852f95862715f92e7d089dc3f2cf"
SRC_URI[sha256sum] = "1b555b8a8800134fdafe32b7d0cb52f5bdbfdd093707c3dd484c5ea59f1d98b7"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit distutils

DEPENDS += " \
        python-pip \
        python-pbr \
        "

RDEPENDS_${PN} += " \
        python-pbr \
        "
