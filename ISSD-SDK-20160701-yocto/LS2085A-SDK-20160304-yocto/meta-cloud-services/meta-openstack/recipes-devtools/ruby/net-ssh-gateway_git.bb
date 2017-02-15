#
# Copyright (C) 2014 Wind River Systems, Inc.
#
SUMMARY = "A library for tunnelling connections to servers via a gateway"
DESCRIPTION = "A library for programmatically tunnelling connections \
to servers via a single “gateway” host. It is useful for establishing \
Net::SSH connections to servers behind firewalls, but can also be used \
to forward ports and establish connections of other types, like HTTP, \
to servers with restricted access."

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE.txt;md5=6c99c0cc0901fbc3607fe997f9898d69"

PR = "r0"

BPV = "1.2.0"
PV = "${BPV}"
SRCREV = "1de7611a7f7cedbe7a4c6cf3798c88d00637582d"

S = "${WORKDIR}/git"

SRC_URI = " \
    git://github.com/net-ssh/net-ssh-gateway.git \
    file://gemspec-don-t-force-the-use-of-gem-private_key.pem.patch \
    "

inherit ruby

RDEPENDS_${PN} += " \
        net-ssh \
        "

BBCLASSEXTEND = "native"
