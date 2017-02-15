#
# Copyright (C) 2014 Wind River Systems, Inc.
#
SUMMARY = "An implementation of the JSON specification according to RFC 4627"
DESCRIPTION = "An implementation of the JSON specification according to RFC 4627"

LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=aef161fe3c86f1a8d6c71d3b8c5655c5"

PR = "r0"

BPV = "1.8.3"
PV = "${BPV}"
SRCREV = "db4c71a7701b95c30f945ee1536240920dcfdc17"

S = "${WORKDIR}/git"

SRC_URI = " \
    git://github.com/flori/json.git \
    "

inherit ruby

BBCLASSEXTEND = "native"
