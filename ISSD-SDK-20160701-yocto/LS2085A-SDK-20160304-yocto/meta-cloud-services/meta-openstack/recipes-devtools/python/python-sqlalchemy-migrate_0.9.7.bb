DESCRIPTION = "Database schema migration for SQLAlchemy"
HOMEPAGE = "http://code.google.com/p/sqlalchemy-migrate/"
SECTION = "devel/python"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://setup.py;beginline=32;endline=32;md5=d41d8cd98f00b204e9800998ecf8427e"

SRCNAME = "sqlalchemy-migrate"

SRC_URI = "https://pypi.python.org/packages/source/s/sqlalchemy-migrate/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "fd586f77b25f905d56d1847fc1f9c68b"
SRC_URI[sha256sum] = "a6adc1c9593e4a4c824230375ce18a887e28e1e9bb5374f5996b58879519f4e5"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

DEPENDS += " \
        python-pip \
        python-pbr \
        "

RDEPENDS_${PN} += "python-sqlalchemy  \
	python-decorator \
	python-tempita  \
        python-pbr \
        python-sqlparse \
    "
