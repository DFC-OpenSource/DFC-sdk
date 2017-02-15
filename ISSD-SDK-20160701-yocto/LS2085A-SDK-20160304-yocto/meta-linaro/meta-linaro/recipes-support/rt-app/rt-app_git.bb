SUMMARRY = "rt-app is a test application that starts multiple periodic threads in order to simulate a real-time periodic load. "

LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING.in;md5=e43fc16fccd8519fba405f0a0ff6e8a3"

PV = "0.1+0.2-alpha2"
SRCREV = "17be4548c4260b80be623e0e1317e98a770dea7a"
SRC_URI = "git://github.com/gbagnoli/rt-app"

S = "${WORKDIR}/git"

inherit autotools

