DESCRIPTION = "Utilities for manipulating POSIX capabilities in Go."
HOMEPAGE = "https://github.com/syndtr/gocapability"
SECTION = "devel/go"
LICENSE = "BSD-2-Clause"
LIC_FILES_CHKSUM = "file://LICENSE;md5=a7304f5073e7be4ba7bffabbf9f2bbca"

PR = "r0"
SRCNAME = "gocapability"

PKG_NAME = "github.com/syndtr/${SRCNAME}"
SRC_URI = "git://${PKG_NAME}.git"

SRCREV = "3c85049eaeb429febe7788d9c7aac42322a377fe"

S = "${WORKDIR}/git"

do_install() {
	install -d ${D}${prefix}/local/go/src/${PKG_NAME}
	cp -r ${S}/* ${D}${prefix}/local/go/src/${PKG_NAME}/
}

SYSROOT_PREPROCESS_FUNCS += "go_mux_sysroot_preprocess"

go_mux_sysroot_preprocess () {
    install -d ${SYSROOT_DESTDIR}${prefix}/local/go/src/${PKG_NAME}
    cp -r ${D}${prefix}/local/go/src/${PKG_NAME} ${SYSROOT_DESTDIR}${prefix}/local/go/src/$(dirname ${PKG_NAME})
}

FILES_${PN} += "${prefix}/local/go/src/${PKG_NAME}/*"
