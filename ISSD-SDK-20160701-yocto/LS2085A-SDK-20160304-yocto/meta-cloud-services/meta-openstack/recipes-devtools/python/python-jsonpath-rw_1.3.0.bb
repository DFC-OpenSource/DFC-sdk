DESCRIPTION = "A robust and significantly extended implementation of JSONPath for Python"
HOMEPAGE = "https://github.com/kennknowles/python-jsonpath-rw"
SECTION = "devel/python"
LICENSE = "BSD+"
LIC_FILES_CHKSUM = "file://README.rst;md5=e3c17535d150260c7235db4e85145fa1"

PR = "r0"
SRCNAME = "jsonpath-rw"

SRC_URI = "http://pypi.python.org/packages/source/j/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "5d1d8d10a21b38637cbd0a84f4b30843"
SRC_URI[sha256sum] = "d4869e5c540e797189acca97f1fef2dfaf5dc3560fd25109d44e353e8eacabbc"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools
