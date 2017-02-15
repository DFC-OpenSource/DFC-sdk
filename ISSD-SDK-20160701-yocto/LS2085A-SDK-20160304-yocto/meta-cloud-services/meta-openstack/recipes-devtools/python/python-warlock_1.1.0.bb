DESCRIPTION = "Build self-validating python objects using JSON schemas"
HOMEPAGE = "http://github.com/bcwaldon/warlock"
SECTION = "devel/python"
LICENSE = "Apache-2"
LIC_FILES_CHKSUM = "file://README.md;md5=97b002bcda83d22e7d3fd1427688a6a6"

DEPENDS += "python-jsonschema python-jsonpatch"

SRCNAME = "warlock"

SRC_URI = "http://pypi.python.org/packages/source/w/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "79649fd8096eeb0c175a193f06237676"
SRC_URI[sha256sum] = "bbfb4279034ccc402723e38d2a2e67cd619988bf4802fda7ba3e8fab15762651"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

RDEPENDS_${PN} += " python-jsonschema python-jsonpatch"

