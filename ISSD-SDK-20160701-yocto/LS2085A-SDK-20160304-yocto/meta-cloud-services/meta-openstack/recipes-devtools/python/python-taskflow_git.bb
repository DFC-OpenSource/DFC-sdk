DESCRIPTION = "A library to complete workflows/tasks in HA manner"
HOMEPAGE = "https://wiki.openstack.org/wiki/TaskFlow"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=4476c4be31402271e101d9a4a3430d52"

SRC_URI = "\
	git://git.openstack.org/openstack/taskflow.git;branch=stable/kilo \
	"

PV="0.7.1+git${SRCPV}"
SRCREV="28bece7c7c97199b8d941b16865c3670c323589a"
S = "${WORKDIR}/git"

inherit setuptools

DEPENDS += " \
        python-pip \
        python-pbr \
        "

RDEPENDS_${PN} += " \
        python-pbr \
        python-anyjson \
        python-iso8601 \
        python-six \
        python-networkx \
        python-babel \
        python-stevedore \
        python-futures \
        python-oslo.utils \
        python-oslo.serialization \
	"
