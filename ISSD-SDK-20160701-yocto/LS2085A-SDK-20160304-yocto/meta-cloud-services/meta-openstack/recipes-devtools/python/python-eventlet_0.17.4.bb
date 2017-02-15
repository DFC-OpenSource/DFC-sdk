DESCRIPTION = "Highly concurrent networking library"
HOMEPAGE = "http://pypi.python.org/pypi/eventlet"
SECTION = "devel/python"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=56472ad6de4caf50e05332a34b66e778"

SRCNAME = "eventlet"

SRC_URI = "http://pypi.python.org/packages/source/e/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "fea0993b14cc7322f944bdd535c4f24a"
SRC_URI[sha256sum] = "8721e9714eaff8d20f2407e0d3a80069db6b57c9226c26ee9db25c541d06556d"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools
