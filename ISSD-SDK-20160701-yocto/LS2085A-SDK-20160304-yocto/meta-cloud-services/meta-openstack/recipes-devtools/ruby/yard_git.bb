#
# Copyright (C) 2014 Wind River Systems, Inc.
#
SUMMARY = "A documentation generation tool for the Ruby programming language."
DESCRIPTION = "YARD is a documentation generation tool for the Ruby \
programming language. It enables the user to generate consistent, \
usable documentation that can be exported to a number of formats very \
easily, and also supports extending for custom Ruby constructs such as \
custom class level definitions."

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=b3e127de4b3f2e58562183d9aba9a7f6"

PR = "r0"

BPV = "0.8.7.3"
PV = "${BPV}"
SRCREV = "b78dea29adafd937f1ca5e813a5269b62ffceba3"

S = "${WORKDIR}/git"

SRC_URI = " \
    git://github.com/lsegal/yard.git \
    "

inherit ruby

BBCLASSEXTEND = "native"