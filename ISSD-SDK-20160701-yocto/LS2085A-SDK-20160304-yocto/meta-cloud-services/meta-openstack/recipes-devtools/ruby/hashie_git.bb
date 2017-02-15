#
# Copyright (C) 2014 Wind River Systems, Inc.
#
SUMMARY = "A collection of tools that extend Hashes and make them more useful."
DESCRIPTION = "A collection of tools that extend Hashes and make them more useful."

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=dd1ec82dd4ea7ed5eebb24bebec9f068"

PR = "r0"

BPV = "3.4.2"
PV = "${BPV}"
SRCREV = "02df8918dd07ef2da1aceba5fd17e8757027345a"

S = "${WORKDIR}/git"

SRC_URI = " \
    git://github.com/intridea/hashie.git \
    "

inherit ruby

BBCLASSEXTEND = "native"
