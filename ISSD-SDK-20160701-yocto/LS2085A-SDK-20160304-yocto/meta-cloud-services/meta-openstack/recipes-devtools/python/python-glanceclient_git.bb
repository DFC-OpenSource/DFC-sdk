DESCRIPTION = "Client library for Glance built on the OpenStack Images API"
HOMEPAGE = "https://github.com/openstack/python-glanceclient"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=34400b68072d710fecd0a2940a0d1658"

SRCREV = "9333d0e4f2b47cf9a0dd52d71bb3a231422b84d2"
PV = "0.17.3+git${SRCPV}"

SRC_URI = "\
	git://github.com/openstack/${BPN}.git;protocol=https;branch=stable/kilo \
	file://fix_glanceclient_memory_leak.patch \
	file://glance-api-check.sh \
	"

S = "${WORKDIR}/git"

inherit setuptools monitor rmargparse

FILES_${PN} += "${datadir}/${SRCNAME}"

DEPENDS += " \
    gmp \
    python-pip \
    python-pbr \
    "

RDEPENDS_${PN} = "gmp \
   python-warlock \
   python-pyopenssl \
   python-prettytable \
   python-setuptools-git \
   python-pbr \
   "

MONITOR_CHECKS_${PN} += "\
	glance-api-check.sh \
"
