#
# Copyright (C) 2014 Wind River Systems, Inc.
#
SUMMARY = "Ruby implementation of the SSH2 client protocol."
DESCRIPTION = "Net::SSH is a pure-Ruby implementation of the SSH2 \
client protocol. It allows you to write programs that invoke and \
interact with processes on remote servers, via SSH2."

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE.txt;md5=6c99c0cc0901fbc3607fe997f9898d69"

PR = "r0"

BPV = "2.9.1"
PV = "${BPV}"
SRCREV = "9f8607984d8e904f211cc5edb39ab2a2ca94008e"

S = "${WORKDIR}/git"

SRC_URI = " \
    git://github.com/net-ssh/net-ssh.git \
    file://gemspec-don-t-force-the-use-of-gem-private_key.pem.patch \
    "

inherit ruby

BBCLASSEXTEND = "native"
