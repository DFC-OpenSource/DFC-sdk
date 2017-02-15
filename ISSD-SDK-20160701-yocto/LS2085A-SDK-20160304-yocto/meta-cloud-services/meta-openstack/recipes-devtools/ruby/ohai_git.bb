#
# Copyright (C) 2014 Wind River Systems, Inc.
#
SUMMARY = "Ohai detects data about your operating system."
DESCRIPTION = "Ohai detects data about your operating system. It can \
be used standalone, but it's primary purpose is to provide node data \
to Chef. Ohai will print out a JSON data blob for all the known data \
about your system. When used with Chef, that data is reported back via \
node attributes."

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=8f7bb094c7232b058c7e9f2e431f389c"

PR = "r0"

BPV = "8.5.0"
PV = "${BPV}"
SRCREV = "5c166cf3fa4b2af541ee54855aae73c809044b3d"

S = "${WORKDIR}/git"

SRC_URI = " \
    git://github.com/opscode/ohai.git \
    "

inherit ruby

RDEPENDS_${PN} += " \
        mime-types \
        ipaddress \
        mixlib-cli \
        mixlib-config \
        mixlib-log \
        mixlib-shellout \
        systemu \
        yajl-ruby \
        "

BBCLASSEXTEND = "native"
