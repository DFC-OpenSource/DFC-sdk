DESCRIPTION = "A collection of tools for internationalizing Python applications"
HOMEPAGE = "http://babel.edgewall.org/"
SECTION = "devel/python"
LICENSE = "BSD"
LIC_FILES_CHKSUM = "file://LICENSE;md5=e6eeaabc92cfc2d03f53e789324d7292"

PR = "r0"
SRCNAME = "Babel"
SRC_URI = "https://pypi.python.org/packages/source/B/${SRCNAME}/${SRCNAME}-${PV}.tar.gz \
"

SRC_URI[md5sum] = "5264ceb02717843cbc9ffce8e6e06bdb"
SRC_URI[sha256sum] = "9f02d0357184de1f093c10012b52e7454a1008be6a5c185ab7a3307aceb1d12e"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

CLEANBROKEN = "1"
