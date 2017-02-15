DESCRIPTION = "Portable network interface information."
HOMEPAGE = "http://alastairs-place.net/netifaces"
SECTION = "devel/python"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

PR = "r0"
SRCNAME = "netifaces"

SRC_URI = "https://pypi.python.org/packages/source/n/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "36da76e2cfadd24cc7510c2c0012eb1e"
SRC_URI[sha256sum] = "9656a169cb83da34d732b0eb72b39373d48774aee009a3d1272b7ea2ce109cde"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools
