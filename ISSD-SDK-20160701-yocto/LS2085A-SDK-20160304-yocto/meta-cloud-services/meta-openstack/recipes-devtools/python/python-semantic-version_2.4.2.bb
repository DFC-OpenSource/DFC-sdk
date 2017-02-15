DESCRIPTION = "A library which provides a few tools to handle SemVer in Python."
HOMEPAGE = "http://pypi.python.org/pypi/semantic_version"
SECTION = "devel/python"
LICENSE = "BSD"
LIC_FILES_CHKSUM = "file://LICENSE;md5=a7dcaa0740d59f8f13ef05a3d0ed7313"

SRCNAME = "semantic_version"
SRC_URI = "http://pypi.python.org/packages/source/s/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "fd7d5ade76e78d8540b9a4044496a57c"
SRC_URI[sha256sum] = "7e8b7fa74a3bc9b6e90b15b83b9bc2377c78eaeae3447516425f475d5d6932d2"

S = "${WORKDIR}/${SRCNAME}-${PV}"

#export BUILD_SYS
#export HOST_SYS

inherit setuptools

# DEPENDS_default: python-pip

DEPENDS += " \
        "

# RDEPENDS_default: 
RDEPENDS_${PN} += " \
        "
