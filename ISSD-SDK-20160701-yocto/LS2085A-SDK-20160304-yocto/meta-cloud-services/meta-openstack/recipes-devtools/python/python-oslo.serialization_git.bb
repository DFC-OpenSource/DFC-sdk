DESCRIPTION = "Oslo Serialization API"
HOMEPAGE = "https://launchpad.net/oslo"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=34400b68072d710fecd0a2940a0d1658"

SRCNAME = "oslo.serialization"
SRC_URI = "git://github.com/openstack/${SRCNAME}.git"

PV = "1.4.0"
SRCREV = "7bfd5dece0f22dbdea1c3e524dbc0eca1f70f1b7"
S = "${WORKDIR}/git"

inherit setuptools

# DEPENDS_default: python-pip

DEPENDS += " \
        python-pip \
        python-pbr \
        "

# RDEPENDS_default: 
RDEPENDS_${PN} += " \
        python-babel \
        python-iso8601 \
        python-oslo.utils \
        python-pbr \
        python-pytz \
        python-six \
        "
