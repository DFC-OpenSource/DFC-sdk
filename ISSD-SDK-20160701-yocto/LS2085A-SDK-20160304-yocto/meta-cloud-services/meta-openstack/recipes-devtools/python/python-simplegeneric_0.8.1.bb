DESCRIPTION = "Simple generic functions"
HOMEPAGE = "https://pypi.python.org/pypi/simplegeneric/"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://README.txt;md5=2085f2c228ed80422edf70e52e86c34b"

PR = "r0"
SRCNAME = "simplegeneric"

SRC_URI = "https://pypi.python.org/packages/source/s/${SRCNAME}/${SRCNAME}-${PV}.zip"

SRC_URI[md5sum] = "f9c1fab00fd981be588fc32759f474e3"
SRC_URI[sha256sum] = "dc972e06094b9af5b855b3df4a646395e43d1c9d0d39ed345b7393560d0b9173"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools
