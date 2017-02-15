DESCRIPTION = "Useful extra bits for Python - things that should be in the standard library"
HOMEPAGE = "https://pypi.python.org/pypi/extras/"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=6d108f338b2f68fe48ac366c4650bd8b"

PR = "r0"
SRCNAME = "extras"

SRC_URI = "https://pypi.python.org/packages/source/e/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "62d8ba049e3386a6df69b413ea81517b"
SRC_URI[sha256sum] = "7a60d84cb661b477c41a5ea35e931ae93860af8cd259ecc0a38a32ef1ae9ffc0"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools
