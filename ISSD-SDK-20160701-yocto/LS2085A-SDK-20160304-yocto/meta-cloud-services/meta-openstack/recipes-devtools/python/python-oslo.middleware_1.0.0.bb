DESCRIPTION = "Oslo Middleware library"
HOMEPAGE = "http://launchpad.net/oslo"
SECTION = "devel/python"
LICENSE = "Apache-2"
LIC_FILES_CHKSUM = "file://LICENSE;md5=34400b68072d710fecd0a2940a0d1658"

SRCNAME = "oslo.middleware"
SRC_URI = "http://pypi.python.org/packages/source/o/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "5102848f299c777e8333d207149e87ab"
SRC_URI[sha256sum] = "5ccf160ae5ce23f3f59b86535352e3b5a9fa35dab0edc4ede8b17438da559995"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

DEPENDS += " \
        python-pip \
        python-pbr \
        "

RDEPENDS_${PN} += " \
        python-babel \
        python-oslo.config \
        python-oslo.context \
        python-oslo.i18n \
        python-six \
        python-stevedore \
        python-webob \
        python-pbr \
        "
