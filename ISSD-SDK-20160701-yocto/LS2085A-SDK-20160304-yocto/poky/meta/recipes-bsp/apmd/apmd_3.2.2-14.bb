SUMMARY = "Utilities for Advanced Power Management"
DESCRIPTION = "The Advanced Power Management (APM) support provides \
access to battery status information and a set of tools for managing \
notebook power consumption."
HOMEPAGE = "http://apenwarr.ca/apmd/"
SECTION = "base"
LICENSE = "GPLv2+"
LIC_FILES_CHKSUM = "file://COPYING;md5=94d55d512a9ba36caa9b7df079bae19f \
                    file://apm.h;beginline=6;endline=18;md5=7d4acc1250910a89f84ce3cc6557c4c2"
DEPENDS = "libtool-cross"
PR = "r2"

SRC_URI = "${DEBIAN_MIRROR}/main/a/apmd/apmd_3.2.2.orig.tar.gz;name=tarball \
           ${DEBIAN_MIRROR}/main/a/apmd/apmd_${PV}.diff.gz;name=patch \
           file://libtool.patch \
           file://unlinux.patch \
           file://init \
           file://default \
           file://apmd_proxy \
           file://apmd_proxy.conf \
           file://apmd.service"

SRC_URI[tarball.md5sum] = "b1e6309e8331e0f4e6efd311c2d97fa8"
SRC_URI[tarball.sha256sum] = "7f7d9f60b7766b852881d40b8ff91d8e39fccb0d1d913102a5c75a2dbb52332d"

SRC_URI[patch.md5sum] = "57e1b689264ea80f78353519eece0c92"
SRC_URI[patch.sha256sum] = "7905ff96be93d725544d0040e425c42f9c05580db3c272f11cff75b9aa89d430"

S = "${WORKDIR}/apmd-3.2.2.orig"

inherit update-rc.d systemd

INITSCRIPT_NAME = "apmd"
INITSCRIPT_PARAMS = "defaults"

SYSTEMD_SERVICE_${PN} = "apmd.service"
SYSTEMD_AUTO_ENABLE = "disable"

do_compile() {
	# apmd doesn't use whole autotools. Just libtool for installation
	oe_runmake "LIBTOOL=${STAGING_BINDIR_CROSS}/${HOST_SYS}-libtool" apm apmd
}

do_install() {
	install -d ${D}${sysconfdir}
	install -d ${D}${sysconfdir}/apm
	install -d ${D}${sysconfdir}/apm/event.d
	install -d ${D}${sysconfdir}/apm/other.d
	install -d ${D}${sysconfdir}/apm/suspend.d
	install -d ${D}${sysconfdir}/apm/resume.d
	install -d ${D}${sysconfdir}/apm/scripts.d
	install -d ${D}${sysconfdir}/default
	install -d ${D}${sysconfdir}/init.d
	install -d ${D}${sbindir}
	install -d ${D}${bindir}
	install -d ${D}${libdir}
	install -d ${D}${datadir}/apmd
	install -d ${D}${includedir}

	install -m 4755 ${S}/.libs/apm ${D}${bindir}/apm
	install -m 0755 ${S}/.libs/apmd ${D}${sbindir}/apmd
	install -m 0755 ${WORKDIR}/apmd_proxy ${D}${sysconfdir}/apm/
	install -m 0644 ${WORKDIR}/apmd_proxy.conf ${D}${datadir}/apmd/
	install -m 0644 ${WORKDIR}/default ${D}${sysconfdir}/default/apmd
	oe_libinstall -so libapm ${D}${libdir}
	install -m 0644 apm.h ${D}${includedir}

	sed -e 's,/usr/sbin,${sbindir},g; s,/etc,${sysconfdir},g;' ${WORKDIR}/init > ${D}${sysconfdir}/init.d/apmd
	chmod 755 ${D}${sysconfdir}/init.d/apmd

	install -d ${D}${systemd_unitdir}/system
	install -m 0644 ${WORKDIR}/apmd.service ${D}${systemd_unitdir}/system/
	sed -i -e 's,@SYSCONFDIR@,${sysconfdir},g' \
		-e 's,@SBINDIR@,${sbindir},g' ${D}${systemd_unitdir}/system/apmd.service
}

PACKAGES =+ "libapm libapm-dev libapm-staticdev apm"

FILES_libapm = "${libdir}/libapm${SOLIBS}"
FILES_libapm-dev = "${libdir}/libapm${SOLIBSDEV} ${includedir} ${libdir}/libapm.la"
FILES_libapm-staticdev = "${libdir}/libapm.a"
FILES_apm = "${bindir}/apm*"
