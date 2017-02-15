DESCRIPTION = "Helper to test WSGI applications"
HOMEPAGE = "https://pypi.python.org/pypi/WebTest/"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://README.rst;md5=7ddcdd3b8e69edc8c5ae7e6bb64f0bc5"

PR = "r0"
SRCNAME = "WebTest"

RDEPENDS_${PN} += "python-beautifulsoup4"

SRC_URI = "https://pypi.python.org/packages/source/W/${SRCNAME}/${SRCNAME}-${PV}.zip"

SRC_URI[md5sum] = "bf0a04fcf8b2cdcaa13b04324cefb53d"
SRC_URI[sha256sum] = "749f527b39893b1757abd0e6814bde811eb25a6cf42538585c971afdb9030dad"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools
