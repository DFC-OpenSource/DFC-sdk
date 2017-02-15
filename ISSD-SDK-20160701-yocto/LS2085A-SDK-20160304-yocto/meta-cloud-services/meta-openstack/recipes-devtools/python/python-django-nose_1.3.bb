#
# Copyright (C) 2014 Wind River Systems, Inc.
#
DESCRIPTION = "Django test runner using nose"
HOMEPAGE = "https://github.com/django-nose/django-nose"
SECTION = "devel/python"
LICENSE = "BSD"
LIC_FILES_CHKSUM = "file://LICENSE;md5=7f88f52f66738ec7259424ce46e855c2"

PR = "r0"
SRCNAME = "django-nose"

SRC_URI = "http://pypi.python.org/packages/source/d/${SRCNAME}/${SRCNAME}-${PV}.tar.gz \ 
          "
SRC_URI[md5sum] = "d39d72ac877cd67af6b41e129911fbfd"
SRC_URI[sha256sum] = "3667d26a41fec30364a0ef72580832ca5328802d553f6d6e72af5ac21cb36365"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

DEPENDS += " \
        python-pip \
        "

RDEPENDS_${PN} += " \
        python-django \
        python-nose \
        "

