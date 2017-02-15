DESCRIPTION = "A golang registry for global request variables."
HOMEPAGE = "https://code.google.com/p/go.net"
SECTION = "devel/go"
LICENSE = "BSD-3-Clause"
LIC_FILES_CHKSUM = "file://LICENSE;md5=5d4950ecb7b26d2c5e4e7b4e0dd74707"

PR = "r0"
SRCNAME = "go.net"

PKG_NAME = "code.google.com/p/${SRCNAME}"
SRC_URI = "hg://code.google.com/p;module=go.net"

SRCREV = "84a4013f96e01fdd14b65d260a78b543e3702ee1"

S = "${WORKDIR}/${SRCNAME}"

do_install() {
	install -d ${D}${prefix}/local/go/src/${PKG_NAME}
	cp -r ${S}/* ${D}${prefix}/local/go/src/${PKG_NAME}/
}

SYSROOT_PREPROCESS_FUNCS += "go_net_sysroot_preprocess"

go_net_sysroot_preprocess () {
    install -d ${SYSROOT_DESTDIR}${prefix}/local/go/src/${PKG_NAME}
    cp -r ${D}${prefix}/local/go/src/${PKG_NAME} ${SYSROOT_DESTDIR}${prefix}/local/go/src/$(dirname ${PKG_NAME})
}

FILES_${PN} += "${prefix}/local/go/src/${PKG_NAME}/*"
