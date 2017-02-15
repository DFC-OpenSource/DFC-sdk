DESCRIPTION = "Fixtures, reusable state for writing clean tests and more"
HOMEPAGE = "https://pypi.python.org/pypi/fixtures/"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://COPYING;md5=239e2f4698b85aad5ed39bae5d2ef226"

PR = "r0"
SRCNAME = "fixtures"

SRC_URI = "https://pypi.python.org/packages/source/f/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "72959be66e26b09641a1e3902f631e62"
SRC_URI[sha256sum] = "b63cf3bb37f83ff815456e2d0e118535ae9a4bf43e76d9a1cf3286041bf717ce"

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
