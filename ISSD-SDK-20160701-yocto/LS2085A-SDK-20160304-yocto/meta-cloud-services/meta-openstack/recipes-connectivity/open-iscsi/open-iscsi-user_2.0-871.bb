DESCRIPTION = "Open-iSCSI project is a high performance, transport independent, multi-platform implementation of RFC3720."
HOMEPAGE = "http://www.open-iscsi.org/"
LICENSE = "GPLv2"
PR = "r1"

inherit systemd

LIC_FILES_CHKSUM = "file://COPYING;md5=393a5ca445f6965873eca0259a17f833"

SRC_URI = "http://www.open-iscsi.org/bits/open-iscsi-${PV}.tar.gz   \
           file://user.patch                                        \
           file://open-iscsi                                        \
           file://initiatorname.iscsi                               \
           "


S = "${WORKDIR}/open-iscsi-${PV}"
TARGET_CC_ARCH += "${LDFLAGS}"


do_compile () {
        oe_runmake user
}

do_install () {
        oe_runmake DESTDIR="${D}" install_user
        cp -f "${WORKDIR}/open-iscsi" "${D}/etc/init.d/"
        install -m 0644 ${WORKDIR}/initiatorname.iscsi ${D}/etc/iscsi/initiatorname.iscsi
}


SRC_URI[md5sum] = "0c403e8c9ad41607571ba0e6e8ff196e"
SRC_URI[sha256sum] = "bcea8746ae82f2ada7bc05d2aa59bcda1ca0d5197f05f2e16744aae59f0a7dcb"

# systemd support
PACKAGES =+ "${PN}-systemd"
SRC_URI_append = "  file://iscsi-initiator                                   \
                    file://iscsi-initiator.service                           \
                    file://iscsi-initiator-targets.service                   \
                 "
RDEPENDS_${PN} += "bash"
RDEPENDS_${PN}-systemd += "${PN}"
FILES_${PN}-systemd +=  "   ${base_libdir}/systemd                  \
                            ${sysconfdir}/default/iscsi-initiator   \
                        "
SYSTEMD_PACKAGES = "${PN}-systemd"
SYSTEMD_SERVICE_${PN}-systemd = "iscsi-initiator.service iscsi-initiator-targets.service"

do_install_append () {
        install -d ${D}${sysconfdir}/default/
        install -m 0644 ${WORKDIR}/iscsi-initiator ${D}${sysconfdir}/default/
        install -d ${D}${systemd_unitdir}/system
        install -m 0644 ${WORKDIR}/iscsi-initiator.service ${D}${systemd_unitdir}/system/
        install -m 0644 ${WORKDIR}/iscsi-initiator-targets.service ${D}${systemd_unitdir}/system/
}
