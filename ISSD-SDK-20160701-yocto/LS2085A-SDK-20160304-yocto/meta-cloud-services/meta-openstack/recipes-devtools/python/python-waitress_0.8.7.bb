DESCRIPTION = "Waitress WSGI server"
HOMEPAGE = "https://pypi.python.org/pypi/waitress/"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE.txt;md5=78ccb3640dc841e1baecb3e27a6966b2"

PR = "r0"
SRCNAME = "waitress"

SRC_URI = "https://pypi.python.org/packages/source/w/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "714f3d458d82a47f12fb168460de8366"
SRC_URI[sha256sum] = "e161e27faa12837739294dd8c837541aa66a5b2764eed753f0bf851d8ac81a74"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools
