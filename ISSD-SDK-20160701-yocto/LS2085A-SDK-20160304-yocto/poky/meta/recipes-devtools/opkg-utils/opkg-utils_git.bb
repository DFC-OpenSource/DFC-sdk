SUMMARY = "Additional utilities for the opkg package manager"
SUMMARY_update-alternatives-opkg = "Utility for managing the alternatives system"
SECTION = "base"
HOMEPAGE = "http://code.google.com/p/opkg/"
LICENSE = "GPLv2+"
LIC_FILES_CHKSUM = "file://COPYING;md5=94d55d512a9ba36caa9b7df079bae19f \
                    file://opkg.py;beginline=1;endline=18;md5=15917491ad6bf7acc666ca5f7cc1e083"
PROVIDES += "virtual/update-alternatives"

SRCREV = "53274f087565fd45d8452c5367997ba6a682a37a"
PV = "0.1.8+git${SRCPV}"

SRC_URI = "git://git.yoctoproject.org/opkg-utils"

S = "${WORKDIR}/git"

TARGET_CC_ARCH += "${LDFLAGS}"

PYTHONRDEPS = "python python-shell python-io python-math python-crypt python-logging python-fcntl python-subprocess python-pickle python-compression python-textutils python-stringold"
PYTHONRDEPS_class-native = ""

PACKAGECONFIG = "python"
PACKAGECONFIG[python] = ",,,${PYTHONRDEPS}"

do_install() {
	oe_runmake PREFIX=${prefix} DESTDIR=${D} install
}

do_install_append_class-target() {
	sed -i ${D}${bindir}/update-alternatives -e 's,/usr/bin,${bindir},g; s,/usr/lib,${libdir},g'
}

PACKAGES =+ "update-alternatives-opkg"
FILES_update-alternatives-opkg = "${bindir}/update-alternatives"
RPROVIDES_update-alternatives-opkg = "update-alternatives update-alternatives-cworth"
RREPLACES_update-alternatives-opkg = "update-alternatives-cworth"
RCONFLICTS_update-alternatives-opkg = "update-alternatives-cworth"

BBCLASSEXTEND = "native nativesdk"
