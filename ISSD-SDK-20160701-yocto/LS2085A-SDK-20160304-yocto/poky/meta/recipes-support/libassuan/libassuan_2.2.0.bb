SUMMARY = "IPC library used by GnuPG and GPGME"
HOMEPAGE = "http://www.gnupg.org/related_software/libassuan/"
BUGTRACKER = "https://bugs.g10code.com/gnupg/index"

LICENSE = "GPLv3 & LGPLv2.1+"
LIC_FILES_CHKSUM = "file://COPYING;md5=f27defe1e96c2e1ecd4e0c9be8967949 \
                    file://COPYING.LIB;md5=2d5025d4aa3495befef8f17206a5b0a1 \
                    file://src/assuan.c;endline=20;md5=0f465544183405055ec179869fc5b5ba \
                    file://src/assuan-defs.h;endline=20;md5=20cd55535260ca1779edae5c7b80b21e"

DEPENDS = "libgpg-error"

SRC_URI = "ftp://ftp.gnupg.org/gcrypt/libassuan/libassuan-${PV}.tar.bz2 \
	   file://libassuan-add-pkgconfig-support.patch"

SRC_URI[md5sum] = "a104faed3e97b9c302c5d67cc22b1d60"
SRC_URI[sha256sum] = "7df58ed70be4b694f77efd1f3b3f103c6311b6b71e04a370382f9fe8204f6ec6"

BINCONFIG = "${bindir}/libassuan-config"

inherit autotools texinfo binconfig-disabled pkgconfig

do_configure_prepend () {
	# Else these could be used in prefernce to those in aclocal-copy
	rm -f ${S}/m4/*.m4
}

BBCLASSEXTEND = "native"
