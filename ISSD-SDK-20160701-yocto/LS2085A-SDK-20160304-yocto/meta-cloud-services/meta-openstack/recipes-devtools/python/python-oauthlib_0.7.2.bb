DESCRIPTION = "A generic, spec-compliant, thorough implementation of the OAuth request-signing logic"
HOMEPAGE = "https://github.com/idan/oauthlib"
SECTION = "devel/python"
LICENSE = "Apache-2"
LIC_FILES_CHKSUM = "file://LICENSE;md5=5ba9ce41463615e082609806255bce1b"

PR = "r1"

SRCNAME = "oauthlib"
SRC_URI = "http://pypi.python.org/packages/source/o/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "eb60abdb002b9c08d248707b79a1cc92"
SRC_URI[sha256sum] = "a051f04ee8ec3305055ab34d87b36c9a449375e07c7d6a05bcafa48329cac7c3"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

# DEPENDS_default: python-pip

DEPENDS += " \
        python-pip \
        "

# RDEPENDS_default: 
RDEPENDS_${PN} += " \
        "

do_install_append() {
        perm_files=`find "${D}${PYTHON_SITEPACKAGES_DIR}/" -name "top_level.txt"`
        for f in $perm_files; do
                chmod 644 "${f}"
        done
}

