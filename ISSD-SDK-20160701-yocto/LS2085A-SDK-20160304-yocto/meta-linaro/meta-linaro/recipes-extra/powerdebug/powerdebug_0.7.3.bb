DESCRIPTION = "tool to display regulator, sensor and clock information"
SUMMARY = "PowerDebug is a tool to display regulator, sensor and clock \
information. Data is refreshed every few seconds. There is also dump option \
to display the information just once."
LICENSE = "EPL-1.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=cdd7b8fa70e88be67e325baae3b8ee95"
DEPENDS = "ncurses"

MMYY = "14.06"
RELEASE = "20${MMYY}"

SRC_URI = "http://releases.linaro.org/${MMYY}/components/power-management/powerdebug/linaro-powerdebug-${PV}-${RELEASE}.tar.bz2"

SRC_URI[md5sum] = "f381946c5ca2f8075ed1bedc5f2a2876"
SRC_URI[sha256sum] = "bfc4c92d8a8a1c8ee208a12310d3c763f111cf76d283facedb7ef56cafd074a1"

S = "${WORKDIR}/powerdebug-${PV}-${RELEASE}"

do_install () {
    install -D -p -m0755 powerdebug ${D}/${sbindir}/powerdebug
    install -D -p -m0644 powerdebug.8 ${D}${mandir}/man8/powerdebug.8
}
