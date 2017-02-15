DESCRIPTION = "QUnit JavaScript library packaged for setuptools"
HOMEPAGE = "https://pypi.python.org/pypi/XStatic-QUnit"
SECTION = "devel/python"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://PKG-INFO;md5=0093d355f8ef8497e548012c01051ce2"

PR = "r0"

SRCNAME = "XStatic-QUnit"
SRC_URI = "http://pypi.python.org/packages/source/X/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "21d48252d3301bd7ae530bfeffa3d108"
SRC_URI[sha256sum] = "c5e2d68d55a3f62b1cfc586112099a522a0a2e2eb22533bbe5dff7d907249ee6"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

# DEPENDS_default: python-pip

DEPENDS += " \
        python-pip \
        "

# RDEPENDS_default: 
RDEPENDS_${PN} += " \
        "
