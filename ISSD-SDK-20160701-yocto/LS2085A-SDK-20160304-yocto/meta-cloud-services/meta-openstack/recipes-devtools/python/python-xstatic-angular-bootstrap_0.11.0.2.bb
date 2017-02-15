DESCRIPTION = "Angular JavaScript library packaged for setuptools"
HOMEPAGE = "https://pypi.python.org/pypi/XStatic-Angular-Bootstrap"
SECTION = "devel/python"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://PKG-INFO;md5=2e4ff6677f43e488434dd77116b7f7ff"

PR = "r0"

SRCNAME = "XStatic-Angular-Bootstrap"
SRC_URI = "http://pypi.python.org/packages/source/X/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "bac078c4e6496367677680c89162ee83"
SRC_URI[sha256sum] = "cbe428bf04c000460776b521f6ace0455e9f3f20135499e9aa2f4af693dc7b3e"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

DEPENDS += " \
        python-pip \
        "

RDEPENDS_${PN} += " \
        "
