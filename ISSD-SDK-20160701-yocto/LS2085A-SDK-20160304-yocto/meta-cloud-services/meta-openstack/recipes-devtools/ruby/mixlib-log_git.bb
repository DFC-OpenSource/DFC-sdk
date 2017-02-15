#
# Copyright (C) 2014 Wind River Systems, Inc.
#
SUMMARY = "Provides a mixin for enabling a class based logger object"
DESCRIPTION = "Provides a mixin for enabling a class based logger \
object, a-la Merb, Chef, and Nanite."

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=8f7bb094c7232b058c7e9f2e431f389c"

PR = "r0"

BPV = "1.4.1"
PV = "${BPV}"
SRCREV = "b750625a79cc46fffe6b886320f96e7874497fa0"

S = "${WORKDIR}/git"

SRC_URI = " \
    git://github.com/opscode/mixlib-log.git \
    "

inherit ruby

BBCLASSEXTEND = "native"
