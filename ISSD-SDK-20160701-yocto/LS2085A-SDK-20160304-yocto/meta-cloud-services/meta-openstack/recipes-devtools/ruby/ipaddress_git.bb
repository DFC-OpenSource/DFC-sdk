#
# Copyright (C) 2014 Wind River Systems, Inc.
#
SUMMARY = "Ruby library designed to make the use of IPv4 and IPv6 addresses simple"
DESCRIPTION = "IPAddress is a Ruby library designed to make the use of \
IPv4 and IPv6 addresses simple, powerful and enjoyable. It provides a \
complete set of methods to handle IP addresses for any need, from \
simple scripting to full network design."

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=52f3a53aa4932dc3fbda4d659f7baa8c"

PR = "r0"

BPV = "0.8.0"
PV = "${BPV}"
# SRCREV is one above release as upstream tag was off
SRCREV = "96aaf68210d644157bd57a6ec3e38c49f38bfc34"

S = "${WORKDIR}/git"

SRC_URI = " \
    git://github.com/bluemonk/ipaddress.git \
    "

inherit ruby

BBCLASSEXTEND = "native"
