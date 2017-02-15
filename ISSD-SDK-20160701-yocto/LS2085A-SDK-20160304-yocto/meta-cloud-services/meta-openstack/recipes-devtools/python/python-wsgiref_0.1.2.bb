DESCRIPTION = "WSGI (PEP 333) Reference Library"
HOMEPAGE = "http://cheeseshop.python.org/pypi/wsgiref"
SECTION = "devel/python"
LICENSE = "PSF"
LIC_FILES_CHKSUM = "file://PKG-INFO;md5=fe761dfec1d54629c6b9a3bbc7b1baf5"

PR = "r0"

SRCNAME = "wsgiref"
SRC_URI = "http://pypi.python.org/packages/source/w/${SRCNAME}/${SRCNAME}-${PV}.zip"

SRC_URI[md5sum] = "29b146e6ebd0f9fb119fe321f7bcf6cb"
SRC_URI[sha256sum] = "c7e610c800957046c04c8014aab8cce8f0b9f0495c8cd349e57c1f7cabf40e79"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

# DEPENDS_default: python-pip

DEPENDS += " \
        python-pip \
        "

# RDEPENDS_default: 
RDEPENDS_${PN} += " \
        "
