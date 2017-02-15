DESCRIPTION = "A messaging framework for Python"
HOMEPAGE = "http://kombu.readthedocs.org"
SECTION = "devel/python"
LICENSE = "BSD"
LIC_FILES_CHKSUM = "file://LICENSE;md5=34442d8097d82121c4a8c1ab10e7d471"

PR = "r0"
SRCNAME = "kombu"

SRC_URI = "https://pypi.python.org/packages/source/k/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "8f33b7e9c4b5757e151b455f51a3a4be"
SRC_URI[sha256sum] = "7da5e90cde3f8c9aea7210345ee15e66da4438bee56087cb0f00e8c7f03dd66a"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

FILES_${PN}-doc += "${datadir}/${SRCNAME}"

RDEPENDS_${PN} = "python-anyjson python-amqplib"
