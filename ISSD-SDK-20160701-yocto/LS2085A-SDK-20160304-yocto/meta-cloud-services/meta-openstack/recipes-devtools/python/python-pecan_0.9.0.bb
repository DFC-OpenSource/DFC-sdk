DESCRIPTION = "WSGI object-dispatching web framework"
HOMEPAGE = "https://pypi.python.org/pypi/pecan/"
SECTION = "devel/python"
LICENSE = "BSD"
LIC_FILES_CHKSUM = "file://LICENSE;md5=d846877d24bbb3d7a00a985c90378e8c"

PR = "r0"
SRCNAME = "pecan"

SRC_URI = "https://pypi.python.org/packages/source/p/${SRCNAME}/${SRCNAME}-${PV}.tar.gz \
"

SRC_URI[md5sum] = "f739a18866a83f6b9ac8622ee89ea784"
SRC_URI[sha256sum] = "d26be0e451c5999504599a9a741534caa0fba97d9bb8991c951f63b79a30b933"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

RDEPENDS_${PN} = "python-webob \
                  python-mako \
                  python-webtest \
                  python-six \
                  python-logutils \
"
