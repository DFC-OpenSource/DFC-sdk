DESCRIPTION = "A collection of Python deprecation patterns and strategies"
HOMEPAGE = "https://pypi.python.org/pypi/debtcollector/"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=34400b68072d710fecd0a2940a0d1658"

SRC_URI[md5sum] = "21e711b88ab956bb1c1d15444bfca0a6"
SRC_URI[sha256sum] = "64cf1ab9bacbdda8c83a569349f2a91211a890973e0119514a9b7bf34518373e"

inherit pypi

DEPENDS += " \
    python-pbr \
    "

RDEPENDS_${PN} += " \
    python-babel \
    python-oslo.utils \
    python-pbr \
    python-six \
    python-wrapt \
    "
