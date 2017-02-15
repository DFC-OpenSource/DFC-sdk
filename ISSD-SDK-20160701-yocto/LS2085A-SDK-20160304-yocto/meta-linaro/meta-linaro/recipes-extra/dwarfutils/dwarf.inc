DEPENDS = "elfutils"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=db2a565b9d860834e0f2c9cf569fb4e5"

SRC_URI = "http://www.prevanders.net/libdwarf-${PV}.tar.gz"

SRC_URI[md5sum] = "83ab49c58439254729f543be6977692b"
SRC_URI[sha256sum] = "c0aede3a7368c27572ae1cc403c4cbdbf6eeb91a05adbb6f41c40ed78b75cdd9"

S = "${WORKDIR}/dwarf-${PV}/${BPN}"

inherit autotools
