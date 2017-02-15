SUMMARY = "Command line tool and library for client-side URL transfers"
HOMEPAGE = "http://curl.haxx.se/"
BUGTRACKER = "http://curl.haxx.se/mail/list.cgi?list=curl-tracker"
SECTION = "console/network"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://COPYING;beginline=7;md5=3a34942f4ae3fbf1a303160714e664ac"

SRC_URI = "http://curl.haxx.se/download/curl-${PV}.tar.bz2 \
           file://pkgconfig_fix.patch \
           file://CVE-2015-3143.patch \
           file://CVE-2015-3144.patch \
           file://CVE-2015-3145.patch \
           file://CVE-2015-3148.patch \
          "

# curl likes to set -g0 in CFLAGS, so we stop it
# from mucking around with debug options
#
SRC_URI += " file://configure_ac.patch"

SRC_URI[md5sum] = "8d30594212e65657a5c32030f0998fa9"
SRC_URI[sha256sum] = "899109eb3900fa6b8a2f995df7f449964292776a04763e94fae640700f883fba"

inherit autotools pkgconfig binconfig multilib_header

PACKAGECONFIG ??= "${@bb.utils.contains("DISTRO_FEATURES", "ipv6", "ipv6", "", d)} gnutls zlib"
PACKAGECONFIG_class-native = "ipv6 ssl zlib"
PACKAGECONFIG_class-nativesdk = "ipv6 ssl zlib"

PACKAGECONFIG[ipv6] = "--enable-ipv6,--disable-ipv6,"
PACKAGECONFIG[ssl] = "--with-ssl --with-random=/dev/urandom,--without-ssl,openssl"
PACKAGECONFIG[gnutls] = "--with-gnutls,--without-gnutls,gnutls"
PACKAGECONFIG[zlib] = "--with-zlib=${STAGING_LIBDIR}/../,--without-zlib,zlib"
PACKAGECONFIG[rtmpdump] = "--with-librtmp,--without-librtmp,rtmpdump"
PACKAGECONFIG[libssh2] = "--with-libssh2,--without-libssh2,libssh2"
PACKAGECONFIG[smb] = "--enable-smb,--disable-smb,"

EXTRA_OECONF = "--without-libidn \
                --enable-crypto-auth \
                --disable-ldap \
                --disable-ldaps \
                --with-ca-bundle=${sysconfdir}/ssl/certs/ca-certificates.crt \
"
# see https://lists.yoctoproject.org/pipermail/poky/2013-December/009435.html
# We should ideally drop ac_cv_sizeof_off_t from site files but until then
EXTRA_OECONF += "${@bb.utils.contains('DISTRO_FEATURES', 'largefile', 'ac_cv_sizeof_off_t=8', '', d)}"

do_install_append() {
	oe_multilib_header curl/curlbuild.h
}

PACKAGES =+ "lib${BPN} lib${BPN}-dev lib${BPN}-staticdev lib${BPN}-doc"

FILES_lib${BPN} = "${libdir}/lib*.so.*"
RRECOMMENDS_lib${BPN} += "ca-certificates"
FILES_lib${BPN}-dev = "${includedir} \
                      ${libdir}/lib*.so \
                      ${libdir}/lib*.la \
                      ${libdir}/pkgconfig \
                      ${datadir}/aclocal \
                      ${bindir}/*-config"
FILES_lib${BPN}-staticdev = "${libdir}/lib*.a"
FILES_lib${BPN}-doc = "${mandir}/man3 \
                      ${mandir}/man1/curl-config.1"

BBCLASSEXTEND = "native nativesdk"
