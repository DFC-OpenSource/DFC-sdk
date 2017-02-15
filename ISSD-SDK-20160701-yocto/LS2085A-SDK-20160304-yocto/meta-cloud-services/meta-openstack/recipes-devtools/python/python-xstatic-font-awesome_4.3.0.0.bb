DESCRIPTION = "Font Awesome icons packaged for setuptools (easy_install) / pip."
HOMEPAGE = "https://pypi.python.org/pypi/XStatic-Font-Awesome"
SECTION = "devel/python"
LICENSE = "Apache-2"
LIC_FILES_CHKSUM = "file://PKG-INFO;md5=821ae461252f11e98fefe8bdfaa130ca"


SRCNAME = "XStatic-Font-Awesome"
SRC_URI = "http://pypi.python.org/packages/source/X/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "56e637fe7b3dcdd2ebd683996ad60759"
SRC_URI[sha256sum] = "d2c0548990a22744ad20013e1d095de8ba1ea56b1878d8393a721205b03885c3"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

# DEPENDS_default: python-pip

DEPENDS += " \
        python-pip \
        "

# RDEPENDS_default: 
RDEPENDS_${PN} += " \
        "
