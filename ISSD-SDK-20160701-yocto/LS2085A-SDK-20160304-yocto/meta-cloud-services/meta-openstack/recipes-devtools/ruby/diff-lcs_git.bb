#
# Copyright (C) 2014 Wind River Systems, Inc.
#
SUMMARY = "Computes the difference between two Enumerable sequences."
DESCRIPTION = "Diff::LCS computes the difference between two \
Enumerable sequences using the McIlroy-Hunt longest common subsequence \
(LCS) algorithm. It includes utilities to create a simple HTML diff \
output format and a standard diff-like tool."

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://License.rdoc;md5=debd9dff6792a920e1ca0ee909e1926a"

PR = "r0"

BPV = "1.2.5"
PV = "${BPV}"
#SRCREV = "d53e92242b9dd6745e56a0ff4ba15d2f62052b91"
SRCREV = "704bc2c0000b5f9bf49d607dcd0d3989b63b2595"

S = "${WORKDIR}/git"

SRC_URI = " \
    git://github.com/halostatue/diff-lcs.git \
    "

inherit ruby

BBCLASSEXTEND = "native"
