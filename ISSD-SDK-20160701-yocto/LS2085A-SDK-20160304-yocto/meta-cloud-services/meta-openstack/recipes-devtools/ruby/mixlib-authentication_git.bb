#
# Copyright (C) 2014 Wind River Systems, Inc.
#
SUMMARY = "Provides a class-based header signing authentication object"
DESCRIPTION = "Mixlib::Authentication provides a class-based header \
signing authentication object, like the one used in Chef."

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=8f7bb094c7232b058c7e9f2e431f389c"

PR = "r0"

BPV = "1.3.0"
PV = "${BPV}"
SRCREV = "db24a56c6f5b99114998a50942220a7023060229"

S = "${WORKDIR}/git"

SRC_URI = " \
    git://github.com/opscode/mixlib-authentication.git \
    "

inherit ruby

RDEPENDS_${PN} += " \
        mixlib-log \
        "

BBCLASSEXTEND = "native"
