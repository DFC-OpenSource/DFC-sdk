DESCRIPTION = "An implementation of JSON Schema validation for Python"
HOMEPAGE = "http://github.com/Julian/jsonschema"
SECTION = "devel/python"
LICENSE = "ISC"
LIC_FILES_CHKSUM = "file://vcversioner.py;beginline=1;endline=13;md5=b20bc154910c13368ad5ba54560f0b0c"

SRCNAME = "vcversioner"

SRC_URI = "http://pypi.python.org/packages/source/v/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "7848a365ced9941053bc25d9a9f8f4b4"
SRC_URI[sha256sum] = "acd43686e92e6c8bbeb4f2eef54408567a7adea9692fa72d591eec5357c03b86"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools
