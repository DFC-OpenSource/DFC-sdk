DESCRIPTION = "Jasmine JavaScript library packaged for setuptools "
HOMEPAGE = "https://pypi.python.org/pypi/XStatic-Jasmine"
SECTION = "devel/python"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://PKG-INFO;md5=eb6b9af5c233043f32ed0225d52cf4a4"


SRCNAME = "XStatic-Jasmine"
SRC_URI = "http://pypi.python.org/packages/source/X/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "63fdb4a8668eb70ceef1a311e8b02dcb"
SRC_URI[sha256sum] = "ba80ab9a324a7abfa5bb4855aaca279aceaf3e0223830d28a17a994770ecc1b4"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

# DEPENDS_default: python-pip

DEPENDS += " \
        python-pip \
        "

# RDEPENDS_default: 
RDEPENDS_${PN} += " \
        "
