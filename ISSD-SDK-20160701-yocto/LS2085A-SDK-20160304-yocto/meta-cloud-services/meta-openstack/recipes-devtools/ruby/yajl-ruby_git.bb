#
# Copyright (C) 2014 Wind River Systems, Inc.
#
SUMMARY = "C binding to the excellent YAJL JSON parsing and generation library."
DESCRIPTION = "This gem is a C binding to the excellent YAJL JSON \
parsing and generation library."

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://MIT-LICENSE;md5=7edc7ac9885163399dacc69a54b1dc3a"

PR = "r0"

BPV = "1.2.0"
PV = "${BPV}"
SRCREV = "fb61daad73340968badf6f84e41ec6931db2e87e"

S = "${WORKDIR}/git"

SRC_URI = " \
    git://github.com/brianmario/yajl-ruby.git \
    file://0001-Don-t-compile-extensions.patch \
    "

inherit ruby

BBCLASSEXTEND = "native"
