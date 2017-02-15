#
# Copyright (C) 2014 Wind River Systems, Inc.
#
SUMMARY = "Erubis is an implementation of eRuby."
DESCRIPTION = "Erubis is an implementation of eRuby."

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://MIT-LICENSE;md5=3014a71019e3adb4bd2c1de02a7195d0"

PR = "r0"

BPV = "2.7.0"
PV = "${BPV}"
SRCREV = "1f0b38d9e66885f8af0244d12d1a6702fc04a8de"

S = "${WORKDIR}/git"

SRC_URI = " \
    git://github.com/kwatch/erubis.git \
    "

inherit ruby

do_patch_append() {
    bb.build.exec_func('do_fixup_gemspec', d)
}

do_fixup_gemspec() {
	sed -e "s:\\\$Release\\\$:${BPV}:g" -i ${S}/${BPN}.gemspec
}

BBCLASSEXTEND = "native"
