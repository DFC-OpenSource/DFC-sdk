SUMMARY = "protobuf-c"
DESCRIPTION = "This package provides a code generator and runtime libraries to use Protocol Buffers from pure C"
HOMEPAGE = "http://code.google.com/p/protobuf-c/"
SECTION = "console/tools"
LICENSE = "Apache-2.0"

LIC_FILES_CHKSUM = "file://src/google/protobuf-c/protobuf-c.c;endline=33;md5=333140fae7cf8a38dc5f980ddb63704b"

PR = "r0"

COMPATIBLE_HOST = "(x86_64|arm).*-linux"

DEPENDS = "protobuf"

SRC_URI[md5sum] = "73ff0c8df50d2eee75269ad8f8c07dc8"
SRC_URI[sha256sum] = "8fcb538e13a5431c46168fc8f2e6ad2574e2db9b684c0c72b066e24f010a0036"
SRC_URI = "http://protobuf-c.googlecode.com/files/protobuf-c-${PV}.tar.gz \
	file://disable_tests.patch"

inherit autotools

BBCLASSEXTEND = "native nativesdk"
