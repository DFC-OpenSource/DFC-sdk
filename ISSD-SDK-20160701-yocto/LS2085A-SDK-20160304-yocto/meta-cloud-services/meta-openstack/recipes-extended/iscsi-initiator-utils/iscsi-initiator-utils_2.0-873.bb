#
# Copyright (C) 2014 Wind River Systems, Inc.
#
LICENSE = "GPLv2 & LGPLv2.1"
LIC_FILES_CHKSUM = \
	"file://COPYING;md5=393a5ca445f6965873eca0259a17f833 \
	 file://utils/open-isns/COPYING;md5=7fbc338309ac38fefcd64b04bb903e34"

PR = "${INC_PR}.0"

SRC_URI[md5sum] = "8b8316d7c9469149a6cc6234478347f7"
SRC_URI[sha256sum] = "7dd9f2f97da417560349a8da44ea4fcfe98bfd5ef284240a2cc4ff8e88ac7cd9"

require ${PN}.inc
