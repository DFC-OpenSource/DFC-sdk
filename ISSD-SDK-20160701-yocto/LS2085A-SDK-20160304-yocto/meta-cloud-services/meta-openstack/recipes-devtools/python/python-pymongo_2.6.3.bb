DESCRIPTION = "Python driver for MongoDB"
HOMEPAGE = "https://pypi.python.org/pypi/pymongo/"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=2a944942e1496af1886903d274dedb13"

PR = "r0"
SRCNAME = "pymongo"

SRC_URI = "https://pypi.python.org/packages/source/p/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "da4a7d6ee47fe30b3978b8805d266167"
SRC_URI[sha256sum] = "cabe1d785ad5db6ed8ff70dcb9c987958fc75400f066ec78911ca3f37184a4e2"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools
