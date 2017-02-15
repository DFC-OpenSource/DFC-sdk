DESCRIPTION = "A database migration tool for SQLAlchemy."
HOMEPAGE = "http://bitbucket.org/zzzeek/alembic"
SECTION = "devel/python"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=8faea05c89b6ff4ad5a9fa082f540719"

SRCNAME = "alembic"

SRC_URI = "https://pypi.python.org/packages/source/a/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "8bd77f40857100da2cdcb6f5da9a7f1c"
SRC_URI[sha256sum] = "abdeded3f92766d30d2e00015f73573e23f96bcb38037fac199a75445e3e66c6"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools 

RDEPENDS_${PN} += "python-sqlalchemy"

