#
# Copyright (C) 2014 Wind River Systems, Inc.
#
SUMMARY = "Provides an interface for developing web applications in Ruby."
DESCRIPTION = "Provides a minimal, modular and adaptable interface for \
developing web applications in Ruby. By wrapping HTTP requests and \
responses in the simplest way possible, it unifies and distills the \
API for web servers, web frameworks, and software in between (the \
so-called middleware) into a single method call."

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://COPYING;md5=ad65b6b9669213d45aa35a0730d58ac2"

PR = "r0"

BPV = "1.6.3"
PV = "${BPV}"
SRCREV = "134d6218d0881d87ae6a522e88eafbddb6cd1bb7"

S = "${WORKDIR}/git"

SRC_URI = " \
    git://github.com/rack/rack.git;branch=1-6-stable \
    "

inherit ruby

BBCLASSEXTEND = "native"
