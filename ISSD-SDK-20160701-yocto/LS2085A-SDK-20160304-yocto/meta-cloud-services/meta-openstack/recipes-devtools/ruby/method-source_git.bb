#
# Copyright (C) 2014 Wind River Systems, Inc.
#
SUMMARY = "A utility to return a method's sourcecode as a Ruby string."
DESCRIPTION = "A utility to return a method's sourcecode as a Ruby string."

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=7d1a6fbb73f604e1e716380490938bd4"

PR = "r0"

BPV = "0.8.2"
PV = "${BPV}"
SRCREV = "1b1f8323a7c25f29331fe32511f50697e5405dbd"

S = "${WORKDIR}/git"

SRC_URI = " \
    git://github.com/banister/method_source.git \
    file://gemspec-bump-version.patch \
    "

inherit ruby

RUBY_BUILD_GEMS = "method_source.gemspec"
RUBY_INSTALL_GEMS = "method_source-${BPV}.gem"

BBCLASSEXTEND = "native"
