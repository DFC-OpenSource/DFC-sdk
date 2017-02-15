DESCRIPTION = "Glance's stores library"
HOMEPAGE = "https://github.com/openstack/glance_store"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=1dece7821bf3fd70fe1309eaa37d52a2"

SRCREV = "ea88e503b617a7ac9a0ae7e537d6517e9992a104"
PV = "0.4.0+git${SRCPV}"

SRC_URI = "\
	git://github.com/openstack/glance_store.git \
	"

S = "${WORKDIR}/git"

inherit setuptools

DEPENDS += " \
        gmp \
        python-pip \
        python-pbr \
        "

RDEPENDS_${PN} = " \
   python-enum34 \
   python-eventlet \
   python-iso8601 \
   python-six \
   python-cinderclient \
   python-oslo.config \
   python-oslo.i18n \
   python-oslo.vmware \
   "

