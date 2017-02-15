DESCRIPTION = "Angular-Mock JavaScript library packaged for setuptools"
HOMEPAGE = "https://pypi.python.org/pypi/XStatic-Angular-Mock/"
SECTION = "devel/python"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://PKG-INFO;md5=31b7650558910a1ee9742e742d4ec810"

PR = "r0"

SRCNAME = "XStatic-Angular-Mock"
SRC_URI = "http://pypi.python.org/packages/source/X/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "94f072c39c2070f3939b619d913fb37f"
SRC_URI[sha256sum] = "ffee6edfab8276abd8057ddc28c4d8503424c0c61938e787720766862ef43e42"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

# DEPENDS_default: python-pip

DEPENDS += " \
        python-pip \
        "

# RDEPENDS_default: 
RDEPENDS_${PN} += " \
        "
