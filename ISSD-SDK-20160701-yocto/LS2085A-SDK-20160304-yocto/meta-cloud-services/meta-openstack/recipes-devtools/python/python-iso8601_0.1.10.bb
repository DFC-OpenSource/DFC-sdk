DESCRIPTION = "Simple module to parse ISO 8601 dates"
HOMEPAGE = "http://code.google.com/p/pyiso8601/"
SECTION = "devel/python"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=ffb8415711cf5d3b081b87c3d0aff737"

PR = "r0"
SRCNAME = "iso8601"

SRC_URI = "http://pypi.python.org/packages/source/i/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "23acb1029acfef9c32069c6c851c3a41"
SRC_URI[sha256sum] = "e712ff3a18604833f5073e836aad795b21170b19bbef70947c441ed89d0ac0e1"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools 
