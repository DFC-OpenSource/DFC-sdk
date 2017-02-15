DESCRIPTION = "Put specified files into rootfs"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COREBASE}/LICENSE;md5=4d92cd373abda3937c2bc47fbc49d690 \
                    file://${COREBASE}/meta/COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"

PACKAGE_ARCH = "${MACHINE_ARCH}"
PR = "r12"

SRC_URI = "file://merge"
S = "${WORKDIR}/merge"

do_install () {
    find ${S}/ -maxdepth 1 -mindepth 1 -not -name README \
    -exec cp -fr '{}' ${D}/ \;
    find ${S}/ -maxdepth 1 -mindepth 1 -exec rm -fr '{}' \;
}
do_unpack[nostamp] = "1"
do_install[nostamp] = "1"

PACKAGES = "${PN}"
FILES_${PN} += "/*"
ALLOW_EMPTY_${PN} = "1"

INHIBIT_PACKAGE_STRIP = "1"
INSANE_SKIP_${PN} = "arch debug-files dev-so"
