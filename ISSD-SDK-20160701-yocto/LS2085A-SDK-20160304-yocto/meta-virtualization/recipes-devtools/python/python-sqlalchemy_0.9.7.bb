DESCRIPTION = "Python SQL toolkit and Object Relational Mapper that gives \
application developers the full power and flexibility of SQL"
HOMEPAGE = "http://www.sqlalchemy.org/"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=6c90a3830628085e8ba945f94d622cb2"
RDEPENDS_${PN} += "python-numbers"

SRCNAME = "SQLAlchemy"
SRC_URI = "https://pypi.python.org/packages/source/S/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "e3c8f836ea9b48886718f59b9d8646ff"
SRC_URI[sha256sum] = "5fff261d0cff21c39913f9d30682659a52bfa2875699b2b7d908d0225df42a15"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools
