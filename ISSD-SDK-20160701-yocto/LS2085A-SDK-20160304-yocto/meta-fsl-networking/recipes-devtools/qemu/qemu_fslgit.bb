require recipes-devtools/qemu/qemu.inc

LIC_FILES_CHKSUM = "file://COPYING;md5=441c28d2cf86e15a37fa47e15a72fbac \
                    file://COPYING.LIB;endline=24;md5=c04def7ae38850e7d3ef548588159913"

# This means QEMU v2.4 with FSL specific patches applied
PV = "2.4+fsl"

RDEPENDS_${PN}_class-target += "nettle gnutls"

PACKAGECONFIG[glx] = ""
PACKAGECONFIG[quorum] = ""
PACKAGECONFIG[vnc-ws] = ""

SRC_URI = "git://sw-stash.freescale.net/scm/sdk/qemu.git;branch=qemu-2.4;protocol=http"
SRC_URI += "file://powerpc_rom.bin"
SRCREV = "5c79ae3615d5dafdf1bb09b7a356a3a005714e3d"

S = "${WORKDIR}/git"

QEMU_TARGETS = "aarch64"

do_configure_prepend() {
    export PKG_CONFIG=${STAGING_DIR_NATIVE}${bindir_native}/pkg-config
}

do_install_append() {
    # Prevent QA Issue about installed ${localstatedir}/run
    if [ -d ${D}${localstatedir}/run ]; then rmdir ${D}${localstatedir}/run; fi
}

# The QEMU version targets for the FSL Silicon
BBCLASSEXTEND = ""
