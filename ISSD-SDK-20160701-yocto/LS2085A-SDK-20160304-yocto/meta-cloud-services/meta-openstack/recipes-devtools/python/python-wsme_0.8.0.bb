DESCRIPTION = "Simplify the writing of REST APIs, and extend them with additional protocols"
HOMEPAGE = "https://pypi.python.org/pypi/WSME/"
SECTION = "devel/python"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=5a9126e7f56a0cf3247050de7f10d0f4"

SRCNAME = "WSME"

SRC_URI = "https://pypi.python.org/packages/source/W/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "8a257dc57d11a750c9aac34033ea8663"
SRC_URI[sha256sum] = "00241e4e4e40d6183f6354a8f5659b601753d49e28b6e5aca332bdf2e31188db"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

DEPENDS += " \
        python-pip \
        python-pbr \
        "

RDEPENDS_${PN} += "\
        python-netaddr \
        python-pbr \
        python-pytz \
        python-simplegeneric \
        python-six \
        python-webob \
        "
