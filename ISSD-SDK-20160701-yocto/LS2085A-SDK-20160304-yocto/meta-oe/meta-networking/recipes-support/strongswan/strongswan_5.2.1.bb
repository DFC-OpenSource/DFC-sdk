DESCRIPTION = "strongSwan is an OpenSource IPsec implementation for the \
Linux operating system."
SUMMARY = "strongSwan is an OpenSource IPsec implementation"
HOMEPAGE = "http://www.strongswan.org"
SECTION = "console/network"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=b234ee4d69f5fce4486a80fdaf4a4263"
DEPENDS = "gmp openssl flex-native flex bison-native"

SRC_URI = "http://download.strongswan.org/strongswan-${PV}.tar.bz2 \
        file://fix-funtion-parameter.patch \
"

SRC_URI[md5sum] = "dd3717c0aa59ab4591ca1812941ebb82"
SRC_URI[sha256sum] = "ea51ab33b5bb39fecaf10668833a9936583b42145948ae9da1ab98f74e939215"

EXTRA_OECONF = "--enable-gmp \
        --enable-openssl \
        --without-lib-prefix \
"

EXTRA_OECONF += "${@base_contains('DISTRO_FEATURES', 'systemd', '--with-systemdsystemunitdir=${systemd_unitdir}/system/', '--without-systemdsystemunitdir', d)}"

PACKAGECONFIG ??= "sqlite3 curl \
        ${@base_contains('DISTRO_FEATURES', 'ldap', 'ldap', '', d)} \
"
PACKAGECONFIG[sqlite3] = "--enable-sqlite,--disable-sqlite,sqlite3,"
PACKAGECONFIG[ldap] = "--enable-ldap,--disable-ldap,openldap,"
PACKAGECONFIG[curl] = "--enable-curl,--disable-curl,curl,"
PACKAGECONFIG[soup] = "--enable-soup,--disable-soup,libsoup-2.4,"
PACKAGECONFIG[mysql] = "--enable-mysql,--disable-mysql,mysql5,"

inherit autotools systemd pkgconfig

RRECOMMENDS_${PN} = "kernel-module-ipsec"

FILES_${PN} += "${libdir}/ipsec/lib*${SOLIBS} ${libdir}/ipsec/plugins/*.so"
FILES_${PN}-dbg += "${libdir}/ipsec/.debug ${libdir}/ipsec/plugins/.debug ${libexecdir}/ipsec/.debug"
FILES_${PN}-dev += "${libdir}/ipsec/lib*${SOLIBSDEV} ${libdir}/ipsec/*.la ${libdir}/ipsec/plugins/*.la"
FILES_${PN}-staticdev += "${libdir}/ipsec/*.a ${libdir}/ipsec/plugins/*.a"

RPROVIDES_${PN} += "${PN}-systemd"
RREPLACES_${PN} += "${PN}-systemd"
RCONFLICTS_${PN} += "${PN}-systemd"
SYSTEMD_SERVICE_${PN} = "${BPN}.service"
