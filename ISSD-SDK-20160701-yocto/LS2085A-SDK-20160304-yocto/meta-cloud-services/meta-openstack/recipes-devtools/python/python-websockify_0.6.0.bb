DESCRIPTION = "WebSockets support for any application/server"
HOMEPAGE = "https://github.com/kanaka/websockify"
SECTION = "devel/python"
LICENSE = "LGPLv3"
LIC_FILES_CHKSUM = "file://LICENSE.txt;md5=a3b5f97c9d64189899b91b3728bfd774"

PR = "r0"
SRCNAME = "websockify"

SRC_URI = "http://pypi.python.org/packages/source/w/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "673a70d007c1a2445c8ef5c7a3067c07"
SRC_URI[sha256sum] = "da4364f54fdcc5350059febe2e8fdf2b53d16cf04ee23c71315e561119f44529"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

RDEPENDS_${PN} += "gmp"

FILES_${PN} += "${datadir}/*"
