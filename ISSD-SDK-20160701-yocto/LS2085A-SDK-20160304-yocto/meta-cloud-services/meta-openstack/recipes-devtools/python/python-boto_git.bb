DESCRIPTION = "Amazon Web Services API"
HOMEPAGE = "https://github.com/boto/boto"
SECTION = "devel/python"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=348302dddd421665d3c9b636a2e55832"

SRCREV = "b5852b0aa5ac91f462b28ac9decee33d872dec4d"
PV = "2.8.0+git${SRCPV}"
PR = "r0"
SRCNAME = "boto"

SRC_URI = "git://github.com/boto/boto.git;protocol=git"

SRC_URI[md5sum] = "5528f3010c42dd0ed7b188a6917295f1"
SRC_URI[sha256sum] = "4d6d38aa8e9e536a27a9737eb4222f896417841fed9a12eedcb619ba8fb68a39"

S = "${WORKDIR}/git"

inherit setuptools

