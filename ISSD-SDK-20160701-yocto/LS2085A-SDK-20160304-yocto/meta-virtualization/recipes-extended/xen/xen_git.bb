require xen.inc

SRCREV = "68bd172e6fa565899c846eb72755c8ffd8562c8a"

PV = "4.4.0+git${SRCPV}"

S = "${WORKDIR}/git"

SRC_URI = " \
    git://xenbits.xen.org/xen.git \
    "

DEFAULT_PREFERENCE = "-1"

PACKAGES += "${PN}-xen-mfndump"

FILES_${PN}-xen-mfndump = "${sbindir}/xen-mfndump"

