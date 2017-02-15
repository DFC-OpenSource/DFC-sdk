DESCRIPTION = "OpenStack Command-line Client"
HOMEPAGE = "https://github.com/openstack/python-openstackclient"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=3b83ef96387f14655fc854ddc3c6bd57"

SRCNAME = "python-openstackclient"

SRC_URI = "http://pypi.python.org/packages/source/p/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "3ba35cbfa48b876d198d83e37a794721"
SRC_URI[sha256sum] = "4ae6d7b35fd1da2a07959fea9639e506871abfb769c62f3692d2f2585605ac85"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

DEPENDS += "\
    python-pbr \
    "

RDEPENDS_${PN} += "\
    python-babel \
    python-cinderclient \
    python-cliff \
    python-cliff-tablib \
    python-glanceclient \
    python-keystoneclient \
    python-neutronclient \
    python-novaclient \
    python-oslo.config \
    python-oslo.i18n \
    python-oslo.serialization \
    python-oslo.utils \
    python-pbr \
    python-requests \
    python-six \
    python-stevedore \
    "
