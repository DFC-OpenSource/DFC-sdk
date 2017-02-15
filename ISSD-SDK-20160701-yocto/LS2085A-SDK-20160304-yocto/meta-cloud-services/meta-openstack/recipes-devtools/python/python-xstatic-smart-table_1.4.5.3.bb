DESCRIPTION = "Angular JavaScript library packaged for setuptools"
HOMEPAGE = "https://pypi.python.org/pypi/XStatic-smart-table"
SECTION = "devel/python"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://PKG-INFO;md5=d59aee3c8bb9698b08aaa3d8df31179e"

PR = "r0"

SRCNAME = "XStatic-smart-table"
SRC_URI = "http://pypi.python.org/packages/source/X/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "fb32bcbfb7ff05cd17ba65aae10946f9"
SRC_URI[sha256sum] = "573bdff0b1ec88dd81b7f92c1b46fda4dd1b92cde94817837d61e62c9b20a8b6"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

DEPENDS += " \
        python-pip \
        "

RDEPENDS_${PN} += " \
        "
