DESCRIPTION = "D3 JavaScript library packaged for setuptools"
HOMEPAGE = "https://pypi.python.org/pypi/XStatic-D3"
SECTION = "devel/python"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://PKG-INFO;md5=0acf9e872391f112ea29deaa9fb0cab5"

PR = "r0"

SRCNAME = "XStatic-D3"
SRC_URI = "http://pypi.python.org/packages/source/X/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "835164e50cfbeb781a42e4d16f75a50c"
SRC_URI[sha256sum] = "46fe521f8dad99f5e20f6702180510c37b81d11f1d78119d308fcec31381f90e"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

# DEPENDS_default: python-pip

DEPENDS += " \
        python-pip \
        "

# RDEPENDS_default: 
RDEPENDS_${PN} += " \
        "
