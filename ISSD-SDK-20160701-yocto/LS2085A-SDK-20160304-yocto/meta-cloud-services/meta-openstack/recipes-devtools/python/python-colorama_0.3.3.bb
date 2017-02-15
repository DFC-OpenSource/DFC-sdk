#
# Copyright (C) 2014 Wind River Systems, Inc.
#
DESCRIPTION = "Simple cross-platform colored terminal text in Python"
HOMEPAGE = "http://pypi.python.org/pypi/colorama"
SECTION = "devel/python"
LICENSE = "BSD"
LIC_FILES_CHKSUM = "file://LICENSE.txt;md5=14d0b64047ed8f510b51ce0495995358"

PR = "r0"
SRCNAME = "colorama"

SRC_URI = "http://pypi.python.org/packages/source/c/${SRCNAME}/${SRCNAME}-${PV}.tar.gz \
          "

SRC_URI[md5sum] = "a56b8dc55158a41ab3c89c4c8feb8824"
SRC_URI[sha256sum] = "eb21f2ba718fbf357afdfdf6f641ab393901c7ca8d9f37edd0bee4806ffa269c"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

DEPENDS += " \
        python-pip \
        "

