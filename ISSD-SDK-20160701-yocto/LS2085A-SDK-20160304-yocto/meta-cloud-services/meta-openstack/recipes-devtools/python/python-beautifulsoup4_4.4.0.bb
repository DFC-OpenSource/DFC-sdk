DESCRIPTION = "Screen-scraping library"
HOMEPAGE = "https://pypi.python.org/pypi/beautifulsoup4/"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://COPYING.txt;md5=39dacabe5494f61c8680f6fa7323b596"

PR = "r0"
SRCNAME = "beautifulsoup4"

SRC_URI = "https://pypi.python.org/packages/source/b/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "63d1f33e6524f408cb6efbc5da1ae8a5"
SRC_URI[sha256sum] = "fad91da88f69438b9ba939ab1b2cabaa31b1d914f1cccb4bb157a993ed2917f6"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

# avoid "error: option --single-version-externally-managed not recognized"
DISTUTILS_INSTALL_ARGS = "--root=${D} \
    --prefix=${prefix} \
    --install-lib=${PYTHON_SITEPACKAGES_DIR} \
    --install-data=${datadir}"
