DESCRIPTION = "Better living through Python with decorators"
HOMEPAGE = "http://pypi.python.org/pypi/decorator"
SECTION = "devel/python"
LICENSE = "BSD"
LIC_FILES_CHKSUM = "file://setup.py;beginline=8;endline=8;md5=08a46ecda64aec8026447390e764b86e"

PR = "r0"
SRCNAME = "decorator"

SRC_URI = "http://pypi.python.org/packages/source/d/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "1e8756f719d746e2fc0dd28b41251356"
SRC_URI[sha256sum] = "c20b404cbb7ee5cebd506688e0114e3cd76f5ce233805a51f36e1a7988d9d783"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools 
