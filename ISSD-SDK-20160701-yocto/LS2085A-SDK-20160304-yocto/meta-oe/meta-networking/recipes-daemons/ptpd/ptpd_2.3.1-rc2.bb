SUMMARY = "The PTP daemon (PTPd)"
DESCRIPTION = "The PTP daemon (PTPd) implements the Precision Time protocol (PTP) as \
defined by the relevant IEEE 1588 standard. PTP Version 1 implements IEEE-1588-2002, \
and PTP Version 2 implements IEEE-1588-2008. PTP was developed to provide very precise \
time coordination of LAN connected computers."
HOMEPAGE = "http://sourceforge.net/projects/ptpd"
SECTION = "network"
LICENSE = "BSD"
LIC_FILES_CHKSUM = "file://README;md5=2452033fe374283f29579898663b1aa8"

DEPENDS = "libpcap"

inherit autotools systemd

# return something like '1.2.3' or '1.2.3/rc1'
#
def get_sub(d):
    parts = d.getVar('PV',True).split('-')
    try:
        return parts[0] + '/' + parts[1]
    except:
        return parts[0]

SRC_URI = "http://downloads.sourceforge.net/project/ptpd/ptpd/${@get_sub(d)}/ptpd-${PV}.tar.gz \
           file://ptpd-add-dpaa-etsec-support.patch \
           file://ptpd-use-pkgconfig.patch \
           file://ptpd.service \
           file://ptpd.conf \
"

SRC_URI[md5sum] = "1ef2f1f2825080a865bbce0eb61246d4"
SRC_URI[sha256sum] = "2802aab758649bb222859dfcb62a5d282709ccb4d3f1df3f26f739cc091d0c8d"

S = "${WORKDIR}/ptpd-${PV}"

EXTRA_OEMAKE = ""

EXTRA_OECONF += "--disable-snmp --with-pcap-config=pkg-config"

do_install() {
    install -d ${D}${bindir} ${D}${mandir}/man8
    install -m 0755 ${B}/src/ptpd2 ${D}${bindir}
    install -m 0644 ${B}/src/ptpd2.8 ${D}${mandir}/man8

    if ${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'true', 'false', d)}; then
        install -d ${D}${systemd_unitdir}/system
        install -m 0644 ${WORKDIR}/ptpd.service ${D}${systemd_unitdir}/system

        sed -i -e 's#@SYSCONFDIR@#${sysconfdir}#g' ${D}${systemd_unitdir}/system/ptpd.service
        sed -i -e 's#@BINDIR@#${bindir}#g' ${D}${systemd_unitdir}/system/ptpd.service

        install -d ${D}${sysconfdir}/default/
        install -m 0644 ${WORKDIR}/ptpd.conf ${D}${sysconfdir}/default/ptpd
    fi
}

SYSTEMD_PACKAGES = "${PN}"
SYSTEMD_SERVICE_${PN} = "ptpd.service"
SYSTEMD_AUTO_ENABLE = "disable"
