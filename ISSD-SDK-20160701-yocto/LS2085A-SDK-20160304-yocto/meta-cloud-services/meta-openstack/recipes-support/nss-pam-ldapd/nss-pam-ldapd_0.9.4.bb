SUMMARY = "NSS and PAM module for using LDAP as a naming service"
DESCRIPTION = "\
 daemon for NSS and PAM lookups using LDAP \
 This package provides a daemon for retrieving user accounts and similar \
 system information from LDAP. It is used by the libnss-ldapd and \
 libpam-ldapd packages but is not very useful by itself. \
 "

HOMEPAGE = "http://arthurdejong.org/nss-pam-ldapd/"

LICENSE = "LGPLv2.1"
LIC_FILES_CHKSUM = "file://COPYING;md5=fbc093901857fcd118f065f900982c24"
SECTION = "base"

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI = "\
	http://arthurdejong.org/nss-pam-ldapd/${PN}-${PV}.tar.gz \
	file://nslcd.init\
	"

inherit autotools


SRC_URI[md5sum] = "0d74202700efdde3b6e551bfff49c132"
SRC_URI[sha256sum] = "fd2e3e0935acfd3d2b13682962f51d28d5855472e690d787e36a476fa40c88e6"

DEPENDS += "libpam openldap"

EXTRA_OECONF = "\
	--disable-pynslcd \
	--libdir=${base_libdir} \
	--with-pam-seclib-dir=${base_libdir}/security \
	"

CONFFILES_${PN} += "${sysconfdir}/nslcd.conf"

FILES_${PN} += "${base_libdir}/security ${datadir}"
FILES_${PN}-dbg += "${base_libdir}/security/.debug"

LDAP_DN ?= "dc=my-domain,dc=com"

do_install_append() {
	install -D -m 0755 ${WORKDIR}/nslcd.init ${D}${sysconfdir}/init.d/nslcd

	sed -i -e 's/^uid nslcd/# uid nslcd/;' ${D}${sysconfdir}/nslcd.conf
	sed -i -e 's/^gid nslcd/# gid nslcd/;' ${D}${sysconfdir}/nslcd.conf
	sed -i -e 's/^base dc=example,dc=com/base ${LDAP_DN}/;' ${D}${sysconfdir}/nslcd.conf
}

inherit update-rc.d

INITSCRIPT_NAME = "nslcd"
INITSCRIPT_PARAMS = "defaults"
