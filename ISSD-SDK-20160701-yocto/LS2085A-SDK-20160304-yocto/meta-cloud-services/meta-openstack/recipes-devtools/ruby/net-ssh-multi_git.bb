#
# Copyright (C) 2014 Wind River Systems, Inc.
#
SUMMARY = "A library for controlling multiple ssh connections via a single interface"
DESCRIPTION = "A library for controlling multiple Net::SSH connections \
via a single interface. It exposes an API similar to that of \
Net::SSH::Connection::Session and Net::SSH::Connection::Channel, \
making it simpler to adapt programs designed for single connections to \
be used with multiple connections."

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE.txt;md5=6c99c0cc0901fbc3607fe997f9898d69"

PR = "r0"

BPV = "1.2.1"
PV = "${BPV}"
SRCREV = "5b668d5ef34102c9ac159a8f21c889fdc7f99f1b"

S = "${WORKDIR}/git"

SRC_URI = " \
    git://github.com/net-ssh/net-ssh-multi.git \
    file://gemspec-don-t-force-the-use-of-gem-private_key.pem.patch \
    "

inherit ruby

RDEPENDS_${PN} += " \
        net-ssh \
        net-ssh-gateway \
        "

BBCLASSEXTEND = "native"
