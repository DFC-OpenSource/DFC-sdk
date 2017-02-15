DESCRIPTION = "Python documentation generator"
HOMEPAGE = "http://sphinx-doc.org/"
SECTION = "devel/python"
LICENSE = "BSD"
LIC_FILES_CHKSUM = "file://LICENSE;md5=da9cf9fa4e0d0076dd4d116777ad401e"

PR = "r0"
SRCNAME = "Sphinx"

SRC_URI = "http://pypi.python.org/packages/source/S/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "67bea6df63be8e2a76ebedc76d8f71a3"
SRC_URI[sha256sum] = "16102b69e939d9ee60b05d694a804a83e3ce047c283f6b981a83573a75ea718d"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools
