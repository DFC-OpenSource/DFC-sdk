SUMMARY = "msmtp is an SMTP client"
DESCRIPTION = "A sendmail replacement for use in MTAs like mutt"
HOMEPAGE = "http://msmtp.sourceforge.net/"
SECTION = "console/network"

LICENSE = "GPLv3"
DEPENDS = "zlib gnutls"

#COPYING or Licence
LIC_FILES_CHKSUM = "file://COPYING;md5=d32239bcb673463ab874e80d47fae504"

SRC_URI = "http://sourceforge.net/projects/msmtp/files/msmtp/${PV}/${BPN}-${PV}.tar.xz \
          "

SRC_URI[md5sum] = "6ebba4809bbc665b8a665a018d1a5ee1"
SRC_URI[sha256sum] = "c45d47f2dc77484e3f74c0f66aef0d5d95de6c47ed71d9f926d86d546f8346f3"

inherit gettext autotools update-alternatives pkgconfig

EXTRA_OECONF += "--without-libsecret --without-libgsasl --without-libidn"

ALTERNATIVE_${PN} = "sendmail"
ALTERNATIVE_TARGET[sendmail] = "${bindir}/msmtp"
ALTERNATIVE_LINK_NAME[sendmail] = "${sbindir}/sendmail"
ALTERNATIVE_PRIORITY = "100"

pkg_postinst_${PN}_linuxstdbase () {
	# /usr/lib/sendmial is required by LSB core test
	[ ! -L $D/usr/lib/sendmail ] && ln -sf ${sbindir}/sendmail $D/usr/lib/
}
