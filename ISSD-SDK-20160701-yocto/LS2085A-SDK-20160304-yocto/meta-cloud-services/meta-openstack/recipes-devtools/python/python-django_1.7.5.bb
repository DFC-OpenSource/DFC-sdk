DESCRIPTION = "A high-level Python Web framework"
HOMEPAGE = "http://www.djangoproject.com/"
SECTION = "devel/python"
LICENSE = "BSD"
LIC_FILES_CHKSUM = "file://LICENSE;md5=fa8608154dcdd4029ae653131d4b7365"

PR = "r1"
SRCNAME = "Django"

SRC_URI = "https://pypi.python.org/packages/source/D/${SRCNAME}/${SRCNAME}-${PV}.tar.gz \
"

SRC_URI[md5sum] = "e33355ee4bb2cbb4ab3954d3dff5eddd"
SRC_URI[sha256sum] = "6ae69c1dfbfc9d0c44ae80e2fbe48e59bbbbb70e8df66ad2b7029bd39947d71d"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit distutils

FILES_${PN} += "${datadir}/django/*"

