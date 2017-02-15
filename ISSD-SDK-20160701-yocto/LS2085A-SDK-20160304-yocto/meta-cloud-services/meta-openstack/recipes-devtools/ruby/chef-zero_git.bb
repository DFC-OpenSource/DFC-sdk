#
# Copyright (C) 2014 Wind River Systems, Inc.
#
SUMMARY = "Chef server that can be useful for chef-solo-like tasks"
DESCRIPTION = "Chef Zero is a simple, easy-install, in-memory Chef \
server that can be useful for Chef Client testing and chef-solo-like \
tasks that require a full Chef Server. It IS intended to be simple, \
Chef 11 compliant, easy to run and fast to start. It is NOT intended \
to be secure, scalable, performant or persistent. It does NO input \
validation, authentication or authorization (it will not throw a 400, \
401 or 403). It does not save data, and will start up empty each time \
you start it."

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=8f7bb094c7232b058c7e9f2e431f389c"

BPV = "4.2.3"
PV = "${BPV}"
SRCREV = "28fe2928469885b0138de4d4270c6eccac8ab482"

S = "${WORKDIR}/git"

SRC_URI = " \
    git://github.com/opscode/chef-zero.git;branch=master \
    "

inherit ruby

RDEPENDS_${PN} += " \
        mixlib-log \
        json \
        hashie \
        mixlib-log \
        rack \
        "

BBCLASSEXTEND = "native"
