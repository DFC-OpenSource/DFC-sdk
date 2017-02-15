def gnome_verdir(v):
    return oe.utils.trim_version(v, 2)

GNOME_COMPRESS_TYPE ?= "bz2"
SECTION ?= "x11/gnome"
GNOMEBN ?= "${BPN}"
SRC_URI = "${GNOME_MIRROR}/${GNOMEBN}/${@gnome_verdir("${PV}")}/${GNOMEBN}-${PV}.tar.${GNOME_COMPRESS_TYPE};name=archive"

DEPENDS += "gnome-common-native"

FILES_${PN} += "${datadir}/application-registry  \
                ${datadir}/mime-info \
                ${datadir}/mime/packages \
                ${datadir}/mime/application \
                ${datadir}/gnome-2.0 \
                ${datadir}/polkit* \
                ${datadir}/GConf \
                ${datadir}/glib-2.0/schemas \
"

FILES_${PN}-doc += "${datadir}/devhelp"

inherit autotools pkgconfig

do_install_append() {
	rm -rf ${D}${localstatedir}/lib/scrollkeeper/*
	rm -rf ${D}${localstatedir}/scrollkeeper/*
	rm -f ${D}${datadir}/applications/*.cache
}

EXTRA_OECONF += "--disable-introspection"

UNKNOWN_CONFIGURE_WHITELIST += "--disable-introspection"
