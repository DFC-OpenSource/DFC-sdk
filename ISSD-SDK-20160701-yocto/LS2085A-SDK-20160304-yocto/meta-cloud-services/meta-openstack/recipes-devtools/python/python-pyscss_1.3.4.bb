DESCRIPTION = "pyScss, a Scss compiler for Python"
HOMEPAGE = "http://github.com/Kronuz/pyScss"
SECTION = "devel/python"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=28155276e8df3f75bbd327335f66f2fa"


SRCNAME = "pyScss"
SRC_URI = "http://pypi.python.org/packages/source/p/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "9527b4864cd6023f77a8277e6fb773d7"
SRC_URI[sha256sum] = "d0323110ecc7d3ead6b99cfec31301306928130e4d0a9eb13226bf390aba8c0e"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

# DEPENDS_default: python-pip

DEPENDS += " \
        python-pip \
        libpcre \
        python-six \
        "

# RDEPENDS_default: 
RDEPENDS_${PN} += " \
        python-enum \
        "

CLEANBROKEN = "1"

