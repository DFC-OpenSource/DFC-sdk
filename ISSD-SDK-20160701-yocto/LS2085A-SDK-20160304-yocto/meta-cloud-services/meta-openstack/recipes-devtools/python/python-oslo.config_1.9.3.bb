DESCRIPTION = "API supporting parsing command line arguments and .ini style configuration files."
HOMEPAGE = "https://pypi.python.org/pypi/oslo.config/1.1.0"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=c46f31914956e4579f9b488e71415ac8"

PR = "r0"
SRCNAME = "oslo.config"

SRC_URI = "https://pypi.python.org/packages/source/o/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "109da1307ea8c65440accccbc59cf9a5"
SRC_URI[sha256sum] = "ab54e67776d9bbee86ba8cce9393ba3186e6e63de926e9797598dc35fe790140"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools rmargparse

DEPENDS += " \
        python-pbr \
        python-pip \
        "

RDEPENDS_${PN} += "python-pbr"
