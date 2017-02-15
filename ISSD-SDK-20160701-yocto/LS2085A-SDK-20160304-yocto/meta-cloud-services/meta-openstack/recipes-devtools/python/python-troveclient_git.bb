DESCRIPTION = "Trove Client Library for OpenStack Datbase service"
HOMEPAGE = "https://github.com/openstack/python-troveclient"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=1dece7821bf3fd70fe1309eaa37d52a2"

PR = "r0"
SRCNAME = "troveclient"

SRC_URI = "\
	git://github.com/openstack/python-troveclient.git;branch=master \
	"

PV = "1.0.9+git${SRCPV}"
SRCREV = "a36ebe539ce0affef5388b7edd7a6a230a0fde6c"
S = "${WORKDIR}/git"

inherit setuptools hosts default_configs


USERADD_PACKAGES = "${PN}"

PROVIDES += " \
    ${SRCNAME} \
    ${SRCNAME}-tests \
    "

PACKAGES += " \
    ${SRCNAME} \
    ${SRCNAME}-bin \
    "

PACKAGES_prepend = " \
    ${SRCNAME}-tests \
    "

FILES_${PN} = " \
    ${libdir}/* \
    "

FILES_${SRCNAME}-tests = " \
    ${libdir}/python*/site-packages/${SRCNAME}/tests/* \
    "

ALLOW_EMPTY_${SRCNAME} = "1"
FILES_${SRCNAME} = " \
    "

FILES_${SRCNAME}-bin = " \
    ${bindir}/* \
    "

DEPENDS += " \
        python-pip \
        python-pbr \
        "

RDEPENDS_${PN} += " \
        python-pbr \
        python-prettytable \
        python-requests \
        python-simplejson \
        python-babel \
        python-six \
	"

RDEPENDS_${SRCNAME}-tests += "python-mock"

RDEPENDS_${SRCNAME} += " \
    ${PN} \
    ${SRCNAME}-bin \
    "

RDEPENDS_${SRCNAME}-bin += " \
    ${PN} \
    "
