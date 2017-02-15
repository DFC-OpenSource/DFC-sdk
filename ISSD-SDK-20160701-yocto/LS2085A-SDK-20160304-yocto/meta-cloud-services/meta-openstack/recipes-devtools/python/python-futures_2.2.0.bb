DESCRIPTION = "Backport of the concurrent.futures package from Python 3.2"
HOMEPAGE = "http://code.google.com/p/pythonfutures"
SECTION = "devel/python"
LICENSE = "BSD"
LIC_FILES_CHKSUM = "file://LICENSE;md5=dd6708d05936d3f6c4e20ed14c87b5e3"

SRCNAME = "futures"
SRC_URI = "http://pypi.python.org/packages/source/f/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "310e446de8609ddb59d0886e35edb534"
SRC_URI[sha256sum] = "151c057173474a3a40f897165951c0e33ad04f37de65b6de547ddef107fd0ed3"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

# DEPENDS_default: python-pip

DEPENDS += " \
        python-pip \
        "

# RDEPENDS_default: 
RDEPENDS_${PN} += " \
        "
