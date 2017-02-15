DESCRIPTION = "Python package for creating and manipulating graphs and networks"
HOMEPAGE = "http://networkx.lanl.gov/"
SECTION = "devel/python"
LICENSE = "BSD"
LIC_FILES_CHKSUM = "file://LICENSE.txt;md5=6bd2e3b81103dff983b4b2d7d3217cf5"

PR = "r0"

SRCNAME = "networkx"
SRC_URI = "http://pypi.python.org/packages/source/n/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "a2d9ee8427c5636426f319968e0af9f2"
SRC_URI[sha256sum] = "6380eb38d0b5770d7e50813c8a48ff7c373b2187b4220339c1adce803df01c59"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

# DEPENDS_default: python-pip

DEPENDS += " \
        python-pip \
        "

# RDEPENDS_default: 
RDEPENDS_${PN} += " \
        "
