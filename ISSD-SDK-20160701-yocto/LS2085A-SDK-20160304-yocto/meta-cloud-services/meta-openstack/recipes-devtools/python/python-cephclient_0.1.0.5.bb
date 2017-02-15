DESCRIPTION = "A client library in python for the Ceph REST API."
HOMEPAGE = "https://github.com/dmsimard/python-cephclient"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=d2794c0df5b907fdace235a619d80314"

PR = "r0"

SRCNAME = "python-cephclient"
SRC_URI = "http://pypi.python.org/packages/source/p/${SRCNAME}/${SRCNAME}-v${PV}.tar.gz"

SRC_URI[md5sum] = "e3746607f1af1573a241b4995c10c938"
SRC_URI[sha256sum] = "9776b786da2aebdc0439ea2f809cc8923f52bc0cf610f77a9c1348466961134b"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

# DEPENDS_default: python-pip

DEPENDS += " \
        python-pip \
        "

# RDEPENDS_default: 
RDEPENDS_${PN} += " \
        python-lxml \
        python-requests \
        python-cython \
        "
