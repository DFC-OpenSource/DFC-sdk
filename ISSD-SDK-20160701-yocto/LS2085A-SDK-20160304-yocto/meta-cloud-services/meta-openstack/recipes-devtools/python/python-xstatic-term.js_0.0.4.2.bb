DESCRIPTION = "Angular JavaScript library packaged for setuptools"
HOMEPAGE = "https://pypi.python.org/pypi/XStatic-term.js/0.0.4.2"
SECTION = "devel/python"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://PKG-INFO;md5=0d4ffb2adc6dcb63bbd77832cf91447e"

PR = "r0"

SRCNAME = "XStatic-term.js"
SRC_URI = "http://pypi.python.org/packages/source/X/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "2224354fa1d98599d9383df5b2e749b7"
SRC_URI[sha256sum] = "1ed5c1cd4de60d6f290a032bfc7cdc4261d8d36cb7788b2b0a610551bbda15ec"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

DEPENDS += " \
        python-pip \
        "

RDEPENDS_${PN} += " \
        "
