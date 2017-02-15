DESCRIPTION = "This package contains the core python parts of NoVNC"
HOMEPAGE = "https://github.com/kanaka/noVNC"
SECTION = "devel/python"

PR = "r0"

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE.txt;md5=6458695fb66dcd893becb5f9f912715e"

SRCREV = "3b8ec46fd26d644e6edbea4f46e630929297e448"
PV = "0.5.1+git${SRCPV}"

SRC_URI = "git://github.com/kanaka/noVNC.git \
           file://python-distutils.patch"

S = "${WORKDIR}/git"

inherit distutils

DEPENDS += " python-websockify"
