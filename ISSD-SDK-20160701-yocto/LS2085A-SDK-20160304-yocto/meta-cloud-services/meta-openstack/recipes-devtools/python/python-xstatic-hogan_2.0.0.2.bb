DESCRIPTION = "Hogan JavaScript library packaged for setuptools"
HOMEPAGE = "https://pypi.python.org/pypi/XStatic-Hogan"
SECTION = "devel/python"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://PKG-INFO;md5=18dc983faa3113478ed59ae157a985d5"

PR = "r0"

SRCNAME = "XStatic-Hogan"
SRC_URI = "http://pypi.python.org/packages/source/X/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "1c1de09c4813e8568aab98fa5270d6a0"
SRC_URI[sha256sum] = "5941bc7fb2a09916b8837848e6fc2a13b2dfc271811e9b522c61e1337d5fc2bd"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

# DEPENDS_default: python-pip

DEPENDS += " \
        python-pip \
        "

# RDEPENDS_default: 
RDEPENDS_${PN} += " \
        "
