DESCRIPTION = "Python-PostgreSQL Database Adapter"
HOMEPAGE = "http://initd.org/psycopg/"
SECTION = "devel/python"
LICENSE = "GPLv3+"
LIC_FILES_CHKSUM = "file://LICENSE;md5=2c9872d13fa571e7ba6de95055da1fe2"
DEPENDS = "postgresql"

PR = "r0"
SRCNAME = "psycopg2"

SRC_URI = "https://pypi.python.org/packages/source/p/${SRCNAME}/${SRCNAME}-${PV}.tar.gz \
           file://remove-pg-config.patch"

SRC_URI[md5sum] = "facd82faa067e99b80146a0ee2f842f6"
SRC_URI[sha256sum] = "6b2f0cc9199de9eaa53ba10ff69b2741e73484b24657e69bdae259561c23bde4"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit distutils
