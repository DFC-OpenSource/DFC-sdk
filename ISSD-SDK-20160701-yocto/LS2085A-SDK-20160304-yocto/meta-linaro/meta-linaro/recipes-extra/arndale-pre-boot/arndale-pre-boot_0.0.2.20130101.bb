DESCRIPTION = "This package contains the BL1 binary for Arndale board, \
a chip-specific pre-bootloader provided by Samsung."
SUMMARY = "Binary pre-bootloader for Arndale (BL1)"
SECTION = "bootloader"
LICENSE = "BSD"
LIC_FILES_CHKSUM = "file://BSD;md5=9c3fd1feed485309afa64b43f98ba22a"

SRCREV = "fddc9ea644ee7d05c439ef7cdecbe20da63cdce3"
PV = "0.0.2.20130101+git${SRCPV}"
PR = "r1"

SRC_URI = "git://git.linaro.org/pkg/arndale-pre-boot.git"

S = "${WORKDIR}/git"

do_install() {
    install -D -p -m0644 arndale-bl1.bin ${D}/lib/firmware/arndale/arndale-bl1.bin
}

PACKAGE_ARCH = "all"

PACKAGES = "${PN}"

FILES_${PN} = "/lib/firmware/arndale/arndale-bl1.bin"
