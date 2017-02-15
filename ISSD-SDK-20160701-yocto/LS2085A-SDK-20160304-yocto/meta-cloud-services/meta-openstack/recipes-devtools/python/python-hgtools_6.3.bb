DESCRIPTION = "Classes and setuptools plugin for Mercurial repositories"
HOMEPAGE = "https://pypi.python.org/pypi/hgtools"
SECTION = "devel/python"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=a428d7abf1504d4dfeb4cbde155ba484"

PR = "r0"
SRCNAME = "hgtools"

SRC_URI = "https://pypi.python.org/packages/source/h/${SRCNAME}/${SRCNAME}-${PV}.zip"

SRC_URI[md5sum] = "584d74b81b1efae3604c53086d1a3acb"
SRC_URI[sha256sum] = "1d0ef6ceaba1673e6923b17d7f09c5ae2f4394d16ef80562812987a27e7836ff"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

