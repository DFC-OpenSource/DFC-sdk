SUMMARY = "Oslo Policy library"
DESCRIPTION = "The Oslo Policy library provides support for RBAC policy enforcement across all OpenStack services."
HOMEPAGE = "https://pypi.python.org/pypi/oslo.policy"
SECTION = "devel/python"
LICENSE = "Apache-2"
LIC_FILES_CHKSUM = "file://LICENSE;md5=1dece7821bf3fd70fe1309eaa37d52a2"


SRCNAME = "oslo.policy"
SRC_URI = "http://pypi.python.org/packages/source/o/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "1612940bb7a6db558563fd5450b75f7e"
SRC_URI[sha256sum] = "e69a5c559f95bcbf91eb0ea9f16aa65f0fe5fccc7fa03693d4cc991b76e969a6"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

DEPENDS += " \
        python-pip \
        python-pbr \
"

RDEPENDS_${PN} += " \
        python-oslo.config \
        python-oslo.i18n \
        python-oslo.serialization \
        python-pbr \
"
