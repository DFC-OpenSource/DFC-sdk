#
# Copyright (C) 2014 Wind River Systems, Inc.
#
SUMMARY = "CodeRay is a Ruby library for syntax highlighting."
DESCRIPTION = "CodeRay is a Ruby library for syntax highlighting."

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://MIT-LICENSE.txt;md5=f622e70ffef94aded92d4aa72c2001b2"

PR = "r0"

BPV = "1.1.0"
PV = "${BPV}"
SRCREV = "a48037b85a12228431b32103786456f36beb355f"

S = "${WORKDIR}/git"

SRC_URI = " \
    git://github.com/rubychan/coderay.git \
    "

inherit ruby

RUBY_COMPILE_FLAGS += "RELEASE=1"

BBCLASSEXTEND = "native"
DEPENDS += "yard"
