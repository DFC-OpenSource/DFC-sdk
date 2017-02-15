DESCRIPTION = "mox3: mock object framework for Python"
HOMEPAGE = "https://pypi.python.org/pypi/mox3/0.9.0"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://COPYING.txt;md5=3b83ef96387f14655fc854ddc3c6bd57"

PR = "r0"

SRCNAME = "mox3"
SRC_URI = "https://pypi.python.org/packages/source/m/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "7574eabaa0f1d8f83289f33aaf2c4884"
SRC_URI[sha256sum] = "8469f35690077f888fbb49a7e31251de9f78bd319ebd4e1af0bbb1349c1ad1be"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

DEPENDS += "\
    python-pbr \
    "

RDEPENDS_${PN} += " \
    python-pbr \
    python-fixtures \
"
