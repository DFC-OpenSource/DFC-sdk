DESCRIPTION = "HTML5 VNC client"
HOMEPAGE = "https://github.com/kanaka/noVNC"
SECTION = "web"

PR = "r0"

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE.txt;md5=ac06308a999996ffc2d24d81b3a39f1b"

SRCREV = "8f12ca7a5a64144fe548cada332d5d19ef26a1fe"
PV = "0.4+git${SRCPV}"

SRC_URI = "git://github.com/kanaka/noVNC.git"

S = "${WORKDIR}/git"

RDEPENDS_${PN} += "python-numpy"
RDEPENDS_${PN} += "python-novnc"
RDEPENDS_${PN} += "python-websockify"

do_compile() {
    :
}

do_install() {
    install -m 755 -d ${D}${bindir}/novnc
    install -m 755 -d ${D}${datadir}/novnc/include

    install -m 644 ${S}/vnc.html ${D}${datadir}/novnc
    install -m 644 ${S}/vnc_auto.html ${D}${datadir}/novnc
    install -m 644 ${S}/images/favicon.ico ${D}${datadir}/novnc
    install -m 644 ${S}/include/base64.js ${D}${datadir}/novnc/include
    install -m 644 ${S}/include/des.js ${D}${datadir}/novnc/include
    install -m 644 ${S}/include/display.js ${D}${datadir}/novnc/include
    install -m 644 ${S}/include/input.js ${D}${datadir}/novnc/include
    install -m 644 ${S}/include/logo.js ${D}${datadir}/novnc/include
    install -m 644 ${S}/include/base.css ${D}${datadir}/novnc/include
    install -m 644 ${S}/include/blue.css ${D}${datadir}/novnc/include
    install -m 644 ${S}/include/black.css ${D}${datadir}/novnc/include
    install -m 644 ${S}/include/playback.js ${D}${datadir}/novnc/include
    install -m 644 ${S}/include/rfb.js ${D}${datadir}/novnc/include
    install -m 644 ${S}/include/ui.js ${D}${datadir}/novnc/include
    install -m 644 ${S}/include/util.js ${D}${datadir}/novnc/include
    install -m 644 ${S}/include/websock.js ${D}${datadir}/novnc/include
    install -m 644 ${S}/include/webutil.js ${D}${datadir}/novnc/include
    install -m 644 ${S}/include/jsunzip.js ${D}${datadir}/novnc/include

    sed -i -e 's:#!/usr/bin/env bash:#!/bin/sh:' ${S}/utils/launch.sh	

    install -m 755 ${S}/utils/launch.sh ${D}${bindir}/novnc-launch.sh
}

