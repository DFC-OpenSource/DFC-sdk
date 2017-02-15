DESCRIPTION = "JSEncrypt JavaScript library packaged for setuptools"
HOMEPAGE = "https://pypi.python.org/pypi/XStatic-JSEncrypt"
SECTION = "devel/python"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://PKG-INFO;md5=42bc00ba34391939fc6c1f4898c187b3"

PR = "r0"

SRCNAME = "XStatic-JSEncrypt"
SRC_URI = "http://pypi.python.org/packages/source/X/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "a2a24f2990500d6709643c3413dd93f8"
SRC_URI[sha256sum] = "5852892afc6f80c7848f4110b6dad190a54aeb908271d67aaeae9d966f4a26b5"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

# DEPENDS_default: python-pip

DEPENDS += " \
        python-pip \
        "

# RDEPENDS_default: 
RDEPENDS_${PN} += " \
        "
