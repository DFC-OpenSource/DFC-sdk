SUMMARY = "This package contains LNG specific configuration"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

RDEPENDS_${PN} = "udev"

SRC_URI = "file://70-persistent-net.rules"

S = "${WORKDIR}"

do_install() {
        install -D -p -m644 ${WORKDIR}/70-persistent-net.rules ${D}/etc/udev/rules.d/70-persistent-net.rules
}
