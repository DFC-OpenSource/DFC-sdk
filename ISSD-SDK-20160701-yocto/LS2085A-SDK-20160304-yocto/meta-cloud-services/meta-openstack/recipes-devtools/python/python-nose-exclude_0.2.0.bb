#
# Copyright (C) 2014 Wind River Systems, Inc.
#
DESCRIPTION = "Nose plugin that allows you to easily specify directories to be excluded from testing."
HOMEPAGE = "http://bitbucket.org/kgrandis/nose-exclude"
SECTION = "devel/python"
LICENSE = "LGPL-2.1"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/LGPL-2.1;md5=1a6d268fd218675ffea8be556788b780"

PR = "r0"
SRCNAME = "nose-exclude"

SRC_URI = "http://pypi.python.org/packages/source/n/${SRCNAME}/${SRCNAME}-${PV}.tar.gz \
          "

SRC_URI[md5sum] = "c8d798c3e29ac82dd5c7bf98a99404af"
SRC_URI[sha256sum] = "27babdc53e0741ed09d21b7fdb5d244aabb1679f67ef81289f0f34e50aca51c9"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

DEPENDS += " \
        python-pip \
        "

RDEPENDS_${PN} += " \
        python-nose \
        "

