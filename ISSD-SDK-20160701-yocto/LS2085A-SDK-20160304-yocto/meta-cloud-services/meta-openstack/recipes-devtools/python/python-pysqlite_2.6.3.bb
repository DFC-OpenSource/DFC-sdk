DESCRIPTION = "DB-API 2.0 interface for SQLite 3.x"
HOMEPAGE = "http://github.com/ghaering/pysqlite"
SECTION = "devel/python"
LICENSE = "Zlib"
LIC_FILES_CHKSUM = "file://LICENSE;md5=a85bd923e5c830f8eb89db354ff72f38"

PR = "r0"

SRCNAME = "pysqlite"
SRC_URI = "http://pypi.python.org/packages/source/p/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "7ff1cedee74646b50117acff87aa1cfa"
SRC_URI[sha256sum] = "fe9c35216bf56c858b34c4b4c8be7e34566ddef29670e5a5b43f9cb8ecfbb28d"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

FILES_${PN} += "${datadir}/*"

# DEPENDS_default: python-pip

DEPENDS += " \
        python-pip \
        "

# RDEPENDS_default: 
RDEPENDS_${PN} += " \
        "
