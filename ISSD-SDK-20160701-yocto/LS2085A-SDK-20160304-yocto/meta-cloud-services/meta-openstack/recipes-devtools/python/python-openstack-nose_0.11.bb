#
# Copyright (C) 2014 Wind River Systems, Inc.
#
DESCRIPTION = "Openstack style output for nosetests"
HOMEPAGE = "https://github.com/jkoelker/openstack-nose"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

PR = "r0"
SRCNAME = "openstack.nose_plugin"

SRC_URI = "http://pypi.python.org/packages/source/o/${SRCNAME}/${SRCNAME}-${PV}.tar.gz \
          "

SRC_URI[md5sum] = "0c6547f413db8c66921b110f78132aac"
SRC_URI[sha256sum] = "a28d44dc23de8164d7893da7020404c73c4325b46d5507911f0257c15f613b4f"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

DEPENDS += " \
        python-pip \
        "

RDEPENDS_${PN} += " \
        python-nose \
        python-colorama \
        python-termcolor \
        "

