DESCRIPTION = "Open-iSCSI project is a high performance, transport independent, multi-platform implementation of RFC3720."
HOMEPAGE = "http://www.open-iscsi.org/"
LICENSE = "GPL"
MACHINE_KERNEL_PR_append = "a"
PR = "r2"

LIC_FILES_CHKSUM = "file://COPYING;md5=393a5ca445f6965873eca0259a17f833"

SRC_URI = " http://www.open-iscsi.org/bits/open-iscsi-${PV}.tar.gz "


S = "${WORKDIR}/open-iscsi-${PV}"
TARGET_CC_ARCH += "${LDFLAGS}"

inherit module

do_compile () {
        oe_runmake 'KSRC=${STAGING_KERNEL_DIR}' LDFLAGS="" kernel
}

do_install() {
        oe_runmake 'KSRC=${STAGING_KERNEL_DIR}' LDFLAGS="" DESTDIR="${D}" install_kernel
}


SRC_URI[md5sum] = "8b8316d7c9469149a6cc6234478347f7"
SRC_URI[sha256sum] = "7dd9f2f97da417560349a8da44ea4fcfe98bfd5ef284240a2cc4ff8e88ac7cd9"
