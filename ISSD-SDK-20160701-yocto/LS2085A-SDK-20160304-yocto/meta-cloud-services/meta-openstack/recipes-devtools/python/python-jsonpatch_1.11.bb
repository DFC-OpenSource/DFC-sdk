DESCRIPTION = "An implementation of the JSON Patch format"
HOMEPAGE = "https://github.com/stefankoegl/python-json-patch"
SECTION = "devel/python"
LICENSE = "BSD+"
LIC_FILES_CHKSUM = "file://jsonpatch.py;beginline=3;endline=30;md5=5cd7d1fbd6b236ed142e4285624f58fe"

PR = "r0"
SRCNAME = "jsonpatch"

SRC_URI = "http://pypi.python.org/packages/source/j/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "9f2d0aa31f99cc97089a203c5bed3924"
SRC_URI[sha256sum] = "22d0bc0f5522a4a03dd9fb4c4cdf7c1f03256546c88be4c61e5ceabd22280e47"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

RDEPENDS_${PN} += "python-jsonpointer"
