#
# Copyright (C) 2014 Wind River Systems, Inc.
#
SUMMARY = "Pry is an alternative to the standard IRB shell for Ruby."
DESCRIPTION = "Pry is a powerful alternative to the standard IRB shell \
for Ruby. It is written from scratch to provide a number of advanced \
features."

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=dde93753687e16ab0c4669232fd27fd0"

PR = "r0"

BPV = "0.10.1"
PV = "${BPV}"
SRCREV = "191dc519813402acd6db0d7f73e652ed61f8111f"

S = "${WORKDIR}/git"

SRC_URI = " \
    git://github.com/pry/pry.git \
    file://rdoc-fixup-opt.banner-heredoc.patch \
    "

inherit ruby

RUBY_COMPILE_FLAGS = ""

RDEPENDS_${PN} += " \
        coderay \
        method-source \
        slop \
        "

BBCLASSEXTEND = "native"
