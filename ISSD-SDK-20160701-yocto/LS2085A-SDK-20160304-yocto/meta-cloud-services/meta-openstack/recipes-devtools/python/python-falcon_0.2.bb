DESCRIPTION = "An unladen web framework for building APIs and app backends."
HOMEPAGE = "http://falconframework.org"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://README.rst;md5=2bf705a48be7b7799862ffe602c94b87"

PR = "r0"

SRCNAME = "falcon"
SRC_URI = "http://pypi.python.org/packages/source/f/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "bf9e8bdd20700f1ff7ce6397cd441fbd"
SRC_URI[sha256sum] = "92bb899bf6e58e2299e3b1de1e628b90c38544ba3354a0141d108318b50c3402"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

# conflicting file prevention
do_install_append() {
	rm -f ${D}${libdir}/python*/site-packages/tests/*
}

DEPENDS += " \
        python-pip \
        "

RDEPENDS_${PN} += " \
        python-six \
        python-mimeparse \
        "
