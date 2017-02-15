DESCRIPTION = "A database migration tool for SQLAlchemy."
HOMEPAGE = "http://bitbucket.org/zzzeek/alembic"
SECTION = "devel/python"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=8faea05c89b6ff4ad5a9fa082f540719"

SRCNAME = "alembic"

SRC_URI = "https://pypi.python.org/packages/source/a/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "1814563f5042b28c5c452f2faa895d8e"
SRC_URI[sha256sum] = "f998184b8cd6d522249e88bb0e7f6ccd80cdd73df50189e85c7c35740b17e7d4"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools 

RDEPENDS_${PN} += "python-sqlalchemy"

