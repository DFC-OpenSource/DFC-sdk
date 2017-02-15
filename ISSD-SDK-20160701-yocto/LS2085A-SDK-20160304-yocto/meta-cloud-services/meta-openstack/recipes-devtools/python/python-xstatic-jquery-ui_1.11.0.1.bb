DESCRIPTION = "%DESCRIPTION%"
HOMEPAGE = "%URL%"
SECTION = "devel/python"
LICENSE = "Apache-2"
LIC_FILES_CHKSUM = "file://PKG-INFO;md5=55ac15f231e5629fb6576e349c318199"

PR = "r0"

SRCNAME = "XStatic-jquery-ui"
SRC_URI = "http://pypi.python.org/packages/source/X/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "03d8ea7a0dab29d548e7bc195703b04f"
SRC_URI[sha256sum] = "099b1836eb0d91b8dc98f5b8a6b856a2631d43af0d47f33ef90ee72ed37bda58"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

# DEPENDS_default: python-pip

DEPENDS += " \
        python-pip \
        "

# RDEPENDS_default: 
RDEPENDS_${PN} += " \
        "
