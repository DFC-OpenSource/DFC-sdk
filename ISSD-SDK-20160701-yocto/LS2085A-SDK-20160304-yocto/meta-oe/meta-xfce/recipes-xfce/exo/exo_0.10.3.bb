DESCRIPTION="Application library for the Xfce desktop environment"
SECTION = "x11"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=b234ee4d69f5fce4486a80fdaf4a4263"
DEPENDS = "gtk+ libxfce4util libxfce4ui virtual/libx11 liburi-perl-native cairo"
DEPENDS_class-native = "glib-2.0-native xfce4-dev-tools-native intltool-native"

inherit xfce pythonnative perlnative gtk-doc

# SRC_URI must follow inherited one
SRC_URI += " \
    file://exo-no-tests-0.8.patch \
    file://configure.patch \
"

SRC_URI_append_class-native = " \
    file://reduce-build-to-exo-csource-only.patch \
"

SRC_URI[md5sum] = "d45b7e18e3a73e4a7b2ec6ef7943de17"
SRC_URI[sha256sum] = "95704b14ef9065268c3aa9a7c0bacbe0ddb490a183a4f5ab13bba4a1722826bd"

PACKAGES =+ "exo-csource"

# Note: python bindings did not work in oe-dev and are about to be moved to
# pyxfce see http://comments.gmane.org/gmane.comp.desktop.xfce.devel.version4/19560
FILES_${PN} += "${datadir}/xfce4/ \
                ${libdir}/xfce4/exo-1 \
"

FILES_${PN}-dbg += "${libdir}/gio/modules/.debug \
"

FILES_exo-csource += "${bindir}/exo-csource"

BBCLASSEXTEND = "native"
