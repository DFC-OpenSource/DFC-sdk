DESCRIPTION = "ASN.1 types and codecs"
HOMEPAGE = "http://sourceforge.net/projects/pyasn1/"
SECTION = "devel/python"
LICENSE = "BSD"
LIC_FILES_CHKSUM = "file://LICENSE;md5=ae098273b2cf8b4af164ac20e32bddf7"

PR = "r0"
SRCNAME = "pyasn1"

SRC_URI = "http://pypi.python.org/packages/source/p/pyasn1/pyasn1-${PV}.tar.gz"

SRC_URI[md5sum] = "2cbd80fcd4c7b1c82180d3d76fee18c8"
SRC_URI[sha256sum] = "e4f81d53c533f6bd9526b047f047f7b101c24ab17339c1a7ad8f98b25c101eab"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools
