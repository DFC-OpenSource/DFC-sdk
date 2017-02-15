#
# Copyright (C) 2014 Wind River Systems, Inc.
#
SUMMARY = "Provides a library and registry for info about MIME content type definitions."
DESCRIPTION = "The mime-types library provides a library and registry \
for information about MIME content type definitions. It can be used to \
determine defined filename extensions for MIME types, or to use \
filename extensions to look up the likely MIME type definitions."

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://Licence.rdoc;md5=ea44698b8e6523aa4ebc4e71e0ed43f1"

PR = "r0"

BPV = "2.5"
PV = "${BPV}"
SRCREV = "bc15d62118b59aabbc9cb6e5734b65bf3bc273f0"

S = "${WORKDIR}/git"

SRC_URI = " \
    git://github.com/halostatue/mime-types.git \
    "

inherit ruby

BBCLASSEXTEND = "native"
