SUMMARY = "Matchbox virtual keyboard for X11"
HOMEPAGE = "http://matchbox-project.org"
BUGTRACKER = "http://bugzilla.yoctoproject.org/"
SECTION = "x11"

LICENSE = "LGPLv2.1"
LIC_FILES_CHKSUM = "file://COPYING;md5=4fbd65380cdd255951079008b364516c \
                    file://src/matchbox-keyboard.h;endline=17;md5=9d6586c69e4a926f3cb0b4425f24ba3c \
                    file://applet/applet.c;endline=18;md5=4a0f721724746b14d95b51ddd42b95e7"

DEPENDS = "libfakekey expat libxft gtk+ matchbox-panel-2"

SRCREV = "217f1bfe14c41cf7e291d04a63aa2d79cc13d063"
PV = "0.0+git${SRCPV}"
PR = "r4"

SRC_URI = "git://git.yoctoproject.org/${BPN};branch=matchbox-keyboard-0-1 \
           file://single-instance.patch \
           file://80matchboxkeyboard.sh"

S = "${WORKDIR}/git"

inherit autotools pkgconfig gettext gtk-immodules-cache

EXTRA_OECONF = "--disable-cairo --enable-gtk-im --enable-applet"

PACKAGES += "${PN}-im ${PN}-applet"

FILES_${PN} = "${bindir}/ \
	       ${sysconfdir} \
	       ${datadir}/applications \
	       ${datadir}/pixmaps \
	       ${datadir}/matchbox-keyboard"

FILES_${PN}-dbg += "${libdir}/gtk-2.0/*/immodules/.debug"

FILES_${PN}-im = "${libdir}/gtk-2.0/*/immodules/*.so"

FILES_${PN}-applet = "${libdir}/matchbox-panel/*.so"


do_install_append () {
	install -d ${D}/${sysconfdir}/X11/Xsession.d/
	install -m 755 ${WORKDIR}/80matchboxkeyboard.sh ${D}/${sysconfdir}/X11/Xsession.d/

	rm -f ${D}${libdir}/gtk-2.0/*/immodules/*.la
	rm -f ${D}${libdir}/matchbox-panel/*.la
}

GTKIMMODULES_PACKAGES = "${PN}-im"

RDEPENDS_${PN} = "formfactor dbus-wait"
RRECOMMENDS_${PN} = "${PN}-applet"
