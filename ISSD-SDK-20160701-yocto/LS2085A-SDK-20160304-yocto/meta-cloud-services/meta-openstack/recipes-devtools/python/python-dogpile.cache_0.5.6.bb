DESCRIPTION = "Python Dogpile Cache: A caching front-end based on the Dogpile lock"
HOMEPAGE = "https://pypi.python.org/pypi/dogpile.cache"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=3ab048e18083e6750668c3d12d2193a0"

PR = "r0"
SRCNAME = "dogpile.cache"

SRC_URI = "https://pypi.python.org/packages/source/d/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "6283f8e0d94f06d75b6987875cb2e6e8"
SRC_URI[sha256sum] = "f80544c5555f66cf7b5fc99f15431f3b35f78009bc6b03b58fe1724236bbc57b"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools


