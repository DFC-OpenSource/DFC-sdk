#
# Copyright (C) 2014 Wind River Systems, Inc.
#
SUMMARY = "Provides a simplified interface to shelling"
DESCRIPTION = "Provides a simplified interface to shelling \
out yet still collecting both standard out and standard error \
and providing full control over environment, working directory, \
uid, gid, etc. No means for passing input to the subprocess is \
provided."

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=8f7bb094c7232b058c7e9f2e431f389c"

PR = "r0"

BPV = "2.1.0"
PV = "${BPV}"
SRCREV = "27ba1e882dcab280527aa1764d1b45aca3ef5961"

S = "${WORKDIR}/git"

SRC_URI = " \
    git://github.com/opscode/mixlib-shellout.git \
    "

inherit ruby

BBCLASSEXTEND = "native"

