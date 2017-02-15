#
# Copyright (C) 2014 Wind River Systems, Inc.
#
SUMMARY = "Universal capture of stdout and stderr and handling of child process pid"
DESCRIPTION = "Universal capture of stdout and stderr and handling of \
child process pid for windows, *nix, etc."

LICENSE = "BSD-2-Clause"
LIC_FILES_CHKSUM = "file://LICENSE;md5=3d813af90843bb649e97414e6d24e3df"

PR = "r0"

BPV = "2.5.2"
PV = "${BPV}"
SRCREV = "cb253a8bf213beea69f27418202e936a22d7308f"

S = "${WORKDIR}/git"

SRC_URI = " \
    git://github.com/ahoward/systemu.git \
    "

inherit ruby

BBCLASSEXTEND = "native"
