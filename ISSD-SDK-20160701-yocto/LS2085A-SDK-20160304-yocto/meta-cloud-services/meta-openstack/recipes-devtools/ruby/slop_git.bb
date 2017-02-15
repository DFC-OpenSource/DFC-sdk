#
# Copyright (C) 2014 Wind River Systems, Inc.
#
SUMMARY = "A simple option parser with an simple syntax and API."
DESCRIPTION = "A simple option parser with an simple syntax and API."

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=40575ded674b04c083ce6818c01f0282"

PR = "r0"

BPV = "4.2.0"
PV = "${BPV}"
SRCREV = "50c4d5a6553c9d0b78dee35a092ea3a40c136fa1"

S = "${WORKDIR}/git"

SRC_URI = " \
    git://github.com/leejarvis/slop.git \
    "

inherit ruby

BBCLASSEXTEND = "native"
