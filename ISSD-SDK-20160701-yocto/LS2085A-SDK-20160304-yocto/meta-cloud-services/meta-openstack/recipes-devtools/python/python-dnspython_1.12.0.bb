DESCRIPTION = "DNS toolkit for Python"
HOMEPAGE = "http://www.dnspython.org/"
SECTION = "devel/python"
LICENSE = "BSD"
LIC_FILES_CHKSUM = "file://LICENSE;md5=397eddfcb4bc7e2ece2fc79724a7cca2"

PR = "r0"
SRCNAME = "dnspython"
SRC_URI = "http://www.dnspython.org/kits/${PV}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "3f2601ef3c8b77fc6d21a9c77a81efeb"
SRC_URI[sha256sum] = "03fb82af866001c4afa58c48027bcc4b80bbf0a7f27e1d861cf06393eea4724f"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

# avoid "error: option --single-version-externally-managed not recognized"
DISTUTILS_INSTALL_ARGS = "--root=${D} \
    --prefix=${prefix} \
    --install-lib=${PYTHON_SITEPACKAGES_DIR} \
    --install-data=${datadir}"
