SUMMARY = "Python higher-order functions and operations on callable objects"
HOMEPAGE = "https://pypi.python.org/pypi/functools32"
SECTION = "devel/python"
LICENSE = "PSFv2"

SRCNAME = "functools32"

LIC_FILES_CHKSUM = "file://LICENSE;md5=27cf2345969ed18e6730e90fb0063a10"
SRC_URI = "http://pypi.python.org/packages/source/f/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"
SRC_URI[md5sum] = "09f24ffd9af9f6cd0f63cb9f4e23d4b2"
SRC_URI[sha256sum] = "f6253dfbe0538ad2e387bd8fdfd9293c925d63553f5813c4e587745416501e6d"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit distutils

DISTUTILS_INSTALL_ARGS = "--root=${D} \
    --prefix=${prefix} \
    --install-lib=${PYTHON_SITEPACKAGES_DIR} \
    --install-data=${datadir}"

DEPENDS += " \
	python-pbr \
	"

RDEPENDS_${PN} += "python-testtools \
	python-pbr \
	"

