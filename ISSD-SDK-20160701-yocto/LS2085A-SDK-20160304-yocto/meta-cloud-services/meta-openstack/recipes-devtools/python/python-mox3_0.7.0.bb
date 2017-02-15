DESCRIPTION = "mox3: mock object framework for Python"
HOMEPAGE = "https://pypi.python.org/pypi/mox3/0.7.0"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://COPYING.txt;md5=3b83ef96387f14655fc854ddc3c6bd57"

PR = "r0"

SRCNAME = "mox3"
SRC_URI = "https://pypi.python.org/packages/source/m/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "3235d9fc504aee015576ca784c16f3ff"
SRC_URI[sha256sum] = "7cc2ffac72d55816bbf670b03cf636b4abdc089c3d8b31a6760b22fc1eeedde2"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

DEPENDS += "\
    python-pbr \
"

RDEPENDS_${PN} += " \
    python-pbr \
    python-fixtures \
"
