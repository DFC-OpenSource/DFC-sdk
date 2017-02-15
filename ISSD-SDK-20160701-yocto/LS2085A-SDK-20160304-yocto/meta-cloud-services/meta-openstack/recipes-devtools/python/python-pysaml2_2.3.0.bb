DESCRIPTION = "Python implementation of SAML Version 2 to be used in a WSGI environment"
HOMEPAGE = "https://github.com/rohe/pysaml2"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://PKG-INFO;md5=dc49f1893f8da88300900a4467475286"

PR = "r0"

SRCNAME = "pysaml2"
SRC_URI = "http://pypi.python.org/packages/source/p/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "9c029de8e94a2cb7b97d7d6eb28126a5"
SRC_URI[sha256sum] = "84e95dac65914d5f506d9c5d5b3a182ed7bc1a4c4181b55597fed0f87d6558fa"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

DEPENDS += " \
        python-pip \
        "

RDEPENDS_${PN} += " \
        "
