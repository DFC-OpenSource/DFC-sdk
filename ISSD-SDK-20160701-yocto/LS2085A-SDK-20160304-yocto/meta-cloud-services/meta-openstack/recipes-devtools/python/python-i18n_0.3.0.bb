DESCRIPTION = "Translation library for Python"
HOMEPAGE = "https://github.com/tuvistavie/python-i18n"
SECTION = "devel/python"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://README.md;md5=7624ac071dec291ca8ef74e62e536a7a"

PR = "r0"

SRCNAME = "python-i18n"
SRC_URI = "http://pypi.python.org/packages/source/p/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "fb6a925052f6927a31d5fd8698e73a36"
SRC_URI[sha256sum] = "f95d5e679e026424e7f75a548d9781daa1fa4454b4fd3ef915942a5615815c23"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

# DEPENDS_default: python-pip

DEPENDS += " \
        python-pip \
        "

# RDEPENDS_default: 
RDEPENDS_${PN} += " \
        python-pyyaml \
        "
