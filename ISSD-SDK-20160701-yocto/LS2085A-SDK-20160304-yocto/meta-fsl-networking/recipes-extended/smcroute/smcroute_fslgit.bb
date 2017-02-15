SUMMARY = "Static Multicast Routing Daemon"
DESCRIPTION = "SMCRoute is a daemon and command line tool \
to manipulate the multicast routing table in the UNIX kernel."
HOMEPAGE = "http://troglobit.com/smcroute.html"
SECTION = "console/network"

LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=4325afd396febcb659c36b49533135d4"

# This means smcroute v2.0.0 with FSL specific patches applied
PV = "2.0.0+fsl"

SRC_URI = "git://sw-stash.freescale.net/scm/dnls2appsoss/smcroute.git;branch=ls2-dev;protocol=http"
SRCREV = "a73405c2d153a20244f80de20b112a1d3d2f20da"

S = "${WORKDIR}/git"

inherit autotools
