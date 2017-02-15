#
# Copyright (C) 2014 Wind River Systems, Inc.
#
SUMMARY = "iSCSI daemon and utility programs"
DESCRIPTION = \
"Open-iSCSI project is a high performance, transport independent, \
multi-platform implementation of RFC3720. The iscsi package provides \
the server daemon for the iSCSI protocol, as well as the utility \
programs used to manage it. iSCSI is a protocol for distributed \
disk access using SCSI commands sent over Internet Protocol networks."
SECTION = "console/network"

HOMEPAGE = "http://www.open-iscsi.org/"
INC_PR = "r4"

UPSTREAM_name = "open-iscsi"

#iscsi-initiator-utils-{use-var-for-config}.patch are from fedora
#change nodes,send_targets... from /etc/iscsi to /var/lib/iscsi
SRC_URI = "http://www.open-iscsi.org/bits/${UPSTREAM_name}-${PV}.tar.gz \
	   file://iscsi-initiator-utils-use-var-for-config.patch \
	   file://iscsi-initiator-utils-dont-use-static.patch	\
	   file://initd.debian	\
"
S = "${WORKDIR}/${UPSTREAM_name}-${PV}"
TARGET_CC_ARCH += "${LDFLAGS}"

#utils/fwparam_ibft/prom_parse.y utils/fwparam_ibft/prom_lex.l
#already parsed in source 2.0 but still depends on flex and bison 
#native in case future version they are not parsed
DEPENDS += "openssl flex-native bison-native"

PACKAGES =+ "${PN}-tests"

FILES_${PN}-tests = "/opt/${BPN}-tests/*"
RDEPENDS_${PN}-tests = "perl"

inherit update-rc.d

do_configure () {
	#need to support cross-compiling in open-isns only
	(cd utils/open-isns; \
	 ./configure --host=${TARGET_SYS} --build=${BUILD_SYS} --with-security=no )
}

do_compile () {
	#make iscsistart one of PROGRAMS if install_user in do_install
	#sed -i -e '/^PROGRAMS = /s;$; usr/iscsistart;' Makefile

	#fix the ar used in open-isns
	sed -i -e 's:ar cr :$(AR) cr :' ${S}/utils/open-isns/Makefile
	oe_runmake user

}

do_install () {
	#completely override the install_user as bugs in Makefile
	#oe_runmake DESTDIR="${D}" install_user

	#install necessary directories
	install -d ${D}${base_sbindir} \
		${D}${sysconfdir}/init.d \
		${D}${sysconfdir}/iscsi \
		${D}${localstatedir}/lib/iscsi/nodes \
		${D}${localstatedir}/lib/iscsi/send_targets \
		${D}${localstatedir}/lib/iscsi/static \
		${D}${localstatedir}/lib/iscsi/isns \
		${D}${localstatedir}/lib/iscsi/slp \
		${D}${localstatedir}/lib/iscsi/ifaces \
		${D}${localstatedir}/lock/iscsi ${D}/${mandir}/man8

	install -p -m 755 ${S}/usr/iscsid ${S}/usr/iscsiadm \
		${S}/utils/iscsi-iname \
		${S}/usr/iscsistart ${D}/${base_sbindir}

	install -p -m 644 ${S}/doc/iscsiadm.8 ${S}/doc/iscsid.8 ${D}/${mandir}/man8
	install -p -m 644 ${S}/etc/iscsid.conf ${D}${sysconfdir}/iscsi
	install -p -m 755 ${WORKDIR}/initd.debian ${D}${sysconfdir}/init.d/iscsid

	#install regression tests
	[ ! -d ${D}/opt/${BPN}-tests ] && mkdir -p ${D}/opt/${BPN}-tests
	install ${S}/test/regression.dat ${S}/test/regression.sh ${D}/opt/${BPN}-tests/

	#open-isns tests
	mkdir -p ${D}/opt/${BPN}-tests/isns-tests
	install ${S}/utils/open-isns/tests/*.pl ${D}/opt/${BPN}-tests/isns-tests/
	install ${S}/utils/open-isns/tests/genkey \
		${S}/utils/open-isns/tests/client.conf \
		${S}/utils/open-isns/tests/server.conf \
		${D}/opt/${BPN}-tests/isns-tests/
	cp -a ${S}/utils/open-isns/tests/data ${D}/opt/${BPN}-tests/isns-tests/
}

pkg_postinst_${PN}() {
	#default there is no initiatorname.iscsi installed
	#but it is needed or iscsid will fail
	
	#will run only when postinst on target
	if [ "x$D" != "x" ]; then
		exit 1
	fi
	if [ ! -f ${sysconfdir}/iscsi/initiatorname.iscsi ]; then
		echo "InitiatorName=$(${base_sbindir}/iscsi-iname)" > \
		${sysconfdir}/iscsi/initiatorname.iscsi
	fi
}

INITSCRIPT_NAME = "iscsid"
INITSCRIPT_PARAMS = "start 30 1 2 3 4 5 . stop 70 0 1 2 3 4 5 6 ."
