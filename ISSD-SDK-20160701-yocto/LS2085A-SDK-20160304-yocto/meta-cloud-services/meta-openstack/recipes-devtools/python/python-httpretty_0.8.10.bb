#
# Copyright (C) 2014 Wind River Systems, Inc.
#
DESCRIPTION = "HTTP client mock for Python"
HOMEPAGE = "https://pypi.python.org/pypi/httpretty"
SECTION = "devel/python"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://COPYING;md5=fac859f0c7aee8d7f10b1aa5b130d1a7"

PR = "r0"
SRCNAME = "httpretty"

SRC_URI = "https://pypi.python.org/packages/source/h/${SRCNAME}/${SRCNAME}-${PV}.tar.gz \
          "

SRC_URI[md5sum] = "9c130b16726cbf85159574ae5761bce7"
SRC_URI[sha256sum] = "474a72722d66841f0e59cee285d837e1c6263be5be7bf2f8e824fc849a99adda"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

DEPENDS += " \
        python-pip \
        "

