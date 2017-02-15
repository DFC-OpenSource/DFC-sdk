require openvswitch.inc

RRECOMMENDS_${PN} += "kernel-module-openvswitch"

PR = "r4"

SRC_URI += "\
	http://openvswitch.org/releases/openvswitch-${PV}.tar.gz \
	file://configure-Only-link-against-libpcap-on-FreeBSD.patch \
	"

SRC_URI[md5sum] = "fe8b49efe9f86b57abab00166b971106"
SRC_URI[sha256sum] = "803966c89d6a5de6d710a2cb4ed73ac8d8111a2c44b12b846dcef8e91ffab167"
LIC_FILES_CHKSUM = "file://COPYING;md5=49eeb5acb1f5e510f12c44f176c42253"

inherit pkgconfig
