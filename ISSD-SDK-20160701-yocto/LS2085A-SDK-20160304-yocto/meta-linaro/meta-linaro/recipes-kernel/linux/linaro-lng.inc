DESCRIPTION = "Linaro LNG Kernel Basic support recipe"
SECTION = "kernel"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=d7810fab7487fb0aad327b76f1be7cd7"
SRCREV = "${AUTOREV}"
SRCREV_FORMAT = "kernel"

inherit kernel siteinfo

S = "${WORKDIR}/git"

COMPATIBLE_MACHINE = "(lng-x86-64|lng-rt-x86-64)"
