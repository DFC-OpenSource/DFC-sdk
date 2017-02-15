#
# Copyright (C) 2014 Wind River Systems, Inc.
#
SUMMARY = "Provides a class-based command line option parsing object"
DESCRIPTION = "Provides a class-based command line option parsing \
object, like the one used in Chef, Ohai and Relish."

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=8f7bb094c7232b058c7e9f2e431f389c"

PR = "r0"

BPV = "1.5.0"
PV = "${BPV}"
SRCREV = "b3b3c12141b5380ec61945770690fc1ae31d92b0"

S = "${WORKDIR}/git"

SRC_URI = " \
    git://github.com/opscode/mixlib-cli.git \
    "

inherit ruby

BBCLASSEXTEND = "native"
