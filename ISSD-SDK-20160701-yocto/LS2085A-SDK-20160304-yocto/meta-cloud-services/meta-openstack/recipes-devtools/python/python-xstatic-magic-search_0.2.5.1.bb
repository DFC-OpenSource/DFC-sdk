DESCRIPTION = "An AngularJS directive that provides a UI for both faceted filtering and as-you-type filtering"
HOMEPAGE = "https://pypi.python.org/pypi/XStatic-Magic-Search"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://PKG-INFO;md5=e3d8469611ec04e52d71f2e293d54e9c"

SRCNAME = "XStatic-Magic-Search"
SRC_URI = "http://pypi.python.org/packages/source/X/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "cfb8a82129fffbe1f5c6339240cb6139"
SRC_URI[sha256sum] = "9b2f35a5792f4e763e6dc319036e3676f3e18f46153096f3ab5e507177ec007e"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

DEPENDS += " \
        "

RDEPENDS_${PN} += " \
        "
