DESCRIPTION = "JQuery.quicksearch JavaScript library packaged for setuptools "
HOMEPAGE = "https://pypi.python.org/pypi/XStatic-JQuery.quicksearch"
SECTION = "devel/python"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://PKG-INFO;md5=503c3857973c52f673691b910068e2d0"

PR = "r0"

SRCNAME = "XStatic-JQuery.quicksearch"
SRC_URI = "http://pypi.python.org/packages/source/X/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "0dc4bd1882cf35dc7b19a236ba09b89d"
SRC_URI[sha256sum] = "1271571b420417add56c274fd935e81bfc79e0d54a03559d6ba5ef369f358477"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

# DEPENDS_default: python-pip

DEPENDS += " \
        python-pip \
        "

# RDEPENDS_default: 
RDEPENDS_${PN} += " \
        "
