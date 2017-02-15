DESCRIPTION = "oslo.concurrency library"
HOMEPAGE = "http://launchpad.net/oslo"
SECTION = "devel/python"
LICENSE = "Apache-2"
LIC_FILES_CHKSUM = "file://LICENSE;md5=34400b68072d710fecd0a2940a0d1658"

SRCNAME = "oslo.concurrency"
SRC_URI = "http://pypi.python.org/packages/source/o/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "1ba5ae8bdbb9c12c1b6baa8e3d400b7f"
SRC_URI[sha256sum] = "149f4c972916f14c14c97d7be7b92ba59497c1bbd09c114ee3681bef0773daca"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

# DEPENDS_default: python-pip

DEPENDS += " \
        python-pip \
        python-pbr \
        "

# RDEPENDS_default: 
RDEPENDS_${PN} += " \
        python-babel \
        python-fixtures \
        python-iso8601 \
        python-oslo.i18n \
        python-oslo.utils \
        python-pbr \
        python-posix-ipc \
        python-retrying \
        python-six \
        "
