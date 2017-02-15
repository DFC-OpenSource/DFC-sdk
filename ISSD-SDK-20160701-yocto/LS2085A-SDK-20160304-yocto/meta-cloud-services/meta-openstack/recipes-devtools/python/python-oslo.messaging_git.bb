DESCRIPTION = "Oslo Messaging API"
HOMEPAGE = "https://launchpad.net/oslo"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=c46f31914956e4579f9b488e71415ac8"

SRCNAME = "oslo.messaging"
SRC_URI = "git://github.com/openstack/${SRCNAME}.git;branch=stable/kilo"

PV = "1.8.3"
SRCREV = "0f24108058fbf15752d384be4c13e4fbac801f2a"
S = "${WORKDIR}/git"

inherit setuptools

DEPENDS += " \
        python-pip \
        python-pbr \
        "

RDEPENDS_${PN} += " \
        bash \
        python-pbr \
        python-oslo.config \
        python-oslo.utils \
        python-oslo.serialization \
        python-oslo.i18n \
        python-stevedore \
        python-six \
        python-eventlet \
        python-pyyaml \
        python-kombu \
        python-oslo.middleware \
        python-futures \
        python-aioeventlet \
        "
