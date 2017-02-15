DESCRIPTION = "SeaBIOS"
HOMEPAGE = "http://www.coreboot.org/SeaBIOS"
LICENSE = "LGPLv3"
SECTION = "firmware"

SRC_URI = " \
    http://code.coreboot.org/p/seabios/downloads/get/${PN}-${PV}.tar.gz \
    file://hostcc.patch \
    file://defconfig \
    "

LIC_FILES_CHKSUM = "file://COPYING;md5=d32239bcb673463ab874e80d47fae504         \
                    file://COPYING.LESSER;md5=6a6a8e020838b23406c81b19c1d46df6  \
                    "

SRC_URI[md5sum] = "3f1e17485ca327b245ae5938d9aa02d9"
SRC_URI[sha256sum] = "858d9eda4ad91efa1c45a5a401d560ef9ca8dd172f03b0a106f06661c252dc51"

PR = "r0"

FILES_${PN} = "/usr/share/firmware"

DEPENDS = "util-linux-native file-native bison-native flex-native gettext-native iasl-native python-native"

TUNE_CCARGS = ""
EXTRA_OEMAKE += "HOSTCC=${BUILD_CC}"
EXTRA_OEMAKE += "CROSS_PREFIX=${TARGET_PREFIX}"

do_configure() {
    install -m 0644 "${WORKDIR}/defconfig" .config
    oe_runmake oldconfig
}

do_compile() {
    unset CPP
    unset CPPFLAGS
    oe_runmake
}

do_install() {
    oe_runmake
    install -d ${D}/usr/share/firmware
    install -m 0644 out/bios.bin ${D}/usr/share/firmware/
}

