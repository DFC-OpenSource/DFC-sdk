DESCRIPTION = "Rickshaw JavaScript library packaged for setuptools"
HOMEPAGE = "https://pypi.python.org/pypi/XStatic-Rickshaw"
SECTION = "devel/python"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://PKG-INFO;md5=af85a1450add1a38e6ff5ca1384cc1b6"

PR = "r0"

SRCNAME = "XStatic-Rickshaw"
SRC_URI = "http://pypi.python.org/packages/source/X/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "789fffdced10e93e10f75ce1ade6fc6c"
SRC_URI[sha256sum] = "147574228757254442700a9eea5150f14acb1224ef0612f896b663ab58406de8"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

# DEPENDS_default: python-pip

DEPENDS += " \
        python-pip \
        "

# RDEPENDS_default: 
RDEPENDS_${PN} += " \
        "
