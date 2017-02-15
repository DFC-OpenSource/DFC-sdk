DESCRIPTION = "Application used to verify network connectivity"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://echo_server.c;endline=30;md5=9b9bc9cb4d5b0d9cea7d9b433a832be1"

SRC_URI = "file://echo-server.tar.gz"

S = "${WORKDIR}/${PN}"

SRC_URI[md5sum] = "4b10c85c5f4cfd37b90f553b0aeaa701"
SRC_URI[sha256sum] = "fbfd05a1a54935c1408dee29d917b124b2d1c1cba9957b29eda2129ca7e3fe2a"

do_install () {
    install -d ${D}${bindir}
    install -m 755 ${S}/echo_server ${D}${bindir}
}
