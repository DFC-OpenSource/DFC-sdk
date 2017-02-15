DESCRIPTION = "Identify specific nodes in a JSON document"
HOMEPAGE = "https://github.com/stefankoegl/python-json-pointer"
SECTION = "devel/python"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://jsonpointer.py;beginline=3;endline=31;md5=5e663c88967b53590856107a043d605c"

PR = "r0"
SRCNAME = "jsonpointer"

SRC_URI = "http://pypi.python.org/packages/source/j/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "c4d3f28e72ba77062538d1c0864c40a9"
SRC_URI[sha256sum] = "39403b47a71aa782de6d80db3b78f8a5f68ad8dfc9e674ca3bb5b32c15ec7308"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools
