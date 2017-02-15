DESCRIPTION = "An implementation of JSON-Schema validation for Python"
HOMEPAGE = "http://pypi.python.org/pypi/jsonschema"
SECTION = "devel/python"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://COPYING;md5=7a60a81c146ec25599a3e1dabb8610a8"

PR = "r0"
SRCNAME = "jsonschema"

SRC_URI = "http://pypi.python.org/packages/source/j/${SRCNAME}/${SRCNAME}-${PV}.zip"

SRC_URI[md5sum] = "e491298485bdefeecb0f1ca3d65e5292"
SRC_URI[sha256sum] = "9088494da4c74497a7a27842ae4ca9c3355b5f7754121edc440463eaf020f079"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

DEPENDS += "python-vcversioner"
