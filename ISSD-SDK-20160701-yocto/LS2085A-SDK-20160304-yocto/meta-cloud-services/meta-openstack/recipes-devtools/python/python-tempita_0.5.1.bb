DESCRIPTION = "A very small text templating language"
HOMEPAGE = "http://pythonpaste.org/tempita/"
SECTION = "devel/python"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://setup.py;beginline=29;endline=29;md5=76f61a7d8ab2ed53293639c86c95ad4b"

PR = "r0"
SRCNAME = "Tempita"

SRC_URI = "https://pypi.python.org/packages/source/T/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "f75bdfeabd2f3755f1ff32d582a574a4"
SRC_URI[sha256sum] = "0ebe6938ca7401db79bac279849fffcb5752029150bcb6f3c3edbe7aa9a077d8"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools
