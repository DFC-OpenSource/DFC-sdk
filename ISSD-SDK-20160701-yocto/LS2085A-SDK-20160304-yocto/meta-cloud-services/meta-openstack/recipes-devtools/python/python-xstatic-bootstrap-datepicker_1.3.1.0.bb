DESCRIPTION = "Bootstrap-Datepicker JavaScript library packaged for setuptools (easy_install) / pip."
HOMEPAGE = "https://pypi.python.org/pypi/XStatic-Bootstrap-Datepicker"
SECTION = "devel/python"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://PKG-INFO;md5=97d9c60d3a8c5fccccb317f944688479"

PR = "r0"

SRCNAME = "XStatic-Bootstrap-Datepicker"
SRC_URI = "http://pypi.python.org/packages/source/X/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "c2f5c58a7b41162923eca5a36b29b8b2"
SRC_URI[sha256sum] = "9edc9b77501001fcee9fbf4bf0a722c263efd928ef928b40081a8269fdd9a944"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

# DEPENDS_default: python-pip

DEPENDS += " \
        python-pip \
        "

# RDEPENDS_default: 
RDEPENDS_${PN} += " \
        "
