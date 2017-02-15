DESCRIPTION = "The Swiss Army knife of Python web development"
HOMEPAGE = "https://pypi.python.org/pypi/Werkzeug/"
SECTION = "devel/python"
LICENSE = "BSD-3-Clause"
LIC_FILES_CHKSUM = "file://LICENSE;md5=ad2e600a437b1b03d25b02df8c23ad1c"

PR = "r0"
SRCNAME = "Werkzeug"

SRC_URI = "https://pypi.python.org/packages/source/W/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "670fad41f57c13b71a6816765765a3dd"
SRC_URI[sha256sum] = "c1baf7a3e8be70f34d931ee173283f406877bd3d17f372bbe82318a5b3c510cc"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

RDEPENDS_${PN} += "python-io \
                   python-datetime \
                   python-email \
                   python-zlib \
                   python-pkgutil \
                   python-html \
                   python-shell \
                   python-pprint \
                   python-subprocess \
                   python-netserver"

CLEANBROKEN = "1"

