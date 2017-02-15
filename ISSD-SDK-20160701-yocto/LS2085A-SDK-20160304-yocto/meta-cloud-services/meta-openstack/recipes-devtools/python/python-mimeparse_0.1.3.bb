DESCRIPTION = "basic functions for parsing mime-type names and matching "
HOMEPAGE = "https://pypi.python.org/pypi/mimeparse/"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://README;md5=07e6feb820fbca7eb99538badb3cd8e2"

PR = "r0"
SRCNAME = "mimeparse"

SRC_URI = "https://pypi.python.org/packages/source/m/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "03ce207391454db37279e78ce2112365"
SRC_URI[sha256sum] = "534ff6feefe1cd03984f444e6415aacc79c0a85f3b868ec41a2fd5003004c31e"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit distutils

DISTUTILS_INSTALL_ARGS = "--root=${D} \
    --prefix=${prefix} \
    --install-lib=${PYTHON_SITEPACKAGES_DIR} \
    --install-data=${datadir}"
