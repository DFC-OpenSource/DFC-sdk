DESCRIPTION = "snort - a free lightweight network intrusion detection system for UNIX and Windows."
HOMEPAGE = "http://www.snort.org/"
LICENSE = "GPL-2.0"
LIC_FILES_CHKSUM = "file://COPYING;md5=78fa8ef966b48fbf9095e13cc92377c5"

DEPENDS = "libpcap libpcre daq libdnet util-linux"

# Blacklist:
#
# http://errors.yoctoproject.org/Errors/Details/8936/
#
# snort failure is again very nasty, because it's m4 which eats all
# available memory and swap before it's killed by OOM killer.
# 
# Luckily it always picked m4
# 
# [Wed Feb 18 19:00:51 2015] Out of memory: Kill process 28522 (m4) score 961 or sacrifice child
# [Wed Feb 18 19:10:51 2015] Out of memory: Kill process 45228 (m4) score 958 or sacrifice child
# ...
PNBLACKLIST[snort] ?= "BROKEN: autotools processing causes OOM condition on configure"

SRC_URI = " ${GENTOO_MIRROR}/${BP}.tar.gz;name=tarball \
            file://snort.init \
            file://disable-inaddr-none.patch \
            file://disable-dap-address-space-id.patch \
            file://0001-libpcap-search-sysroot-for-headers.patch \
            file://not-hardcoded-libdir.patch \
"

SRC_URI[tarball.md5sum] = "18111f6de3989ca89add36077a7c2659"
SRC_URI[tarball.sha256sum] = "3cc6c8a9b52f4c863a5736a73b4012aff340b50b5e002771b04d4877f47cd19e"

inherit autotools gettext update-rc.d pkgconfig

INITSCRIPT_NAME = "snort"
INITSCRIPT_PARAMS = "defaults"

EXTRA_OECONF = " \
	--enable-gre \
	--enable-linux-smp-stats \
	--enable-reload \
	--enable-reload-error-restart \
	--enable-targetbased \
	--disable-static-daq \
	--with-dnet-includes=${STAGING_INCDIR} \
	--with-dnet-libraries=${STAGING_LIBDIR} \
	"

# if you want to disable it, you need to patch configure.in first
# AC_CHECK_HEADERS([openssl/sha.h],, SHA_H="no")
# is called even with --without-openssl-includes
PACKAGECONFIG ?= "openssl"
PACKAGECONFIG[openssl] = "--with-openssl-includes=${STAGING_INCDIR} --with-openssl-libraries=${STAGING_LIBDIR}, --without-openssl-includes --without-openssl-libraries, openssl,"

do_install_append() {
    install -d ${D}/${sysconfdir}/snort/rules
    install -d ${D}/${sysconfdir}/snort/preproc_rules
    install -d ${D}${sysconfdir}/init.d
    for i in map config conf dtd; do
        cp ${S}/etc/*.$i ${D}/${sysconfdir}/snort/
    done
    cp ${S}/preproc_rules/*.rules ${D}/${sysconfdir}/snort/preproc_rules/
    install -m 755 ${WORKDIR}/snort.init ${D}/${sysconfdir}/init.d/snort
    mkdir -p ${D}/${localstatedir}/log/snort
    install -d ${D}/var/log/snort
}

FILES_${PN} += " \
	${libdir}/snort_dynamicengine/*.so.* \
	${libdir}/snort_dynamicpreprocessor/*.so.* \
	${libdir}/snort_dynamicrules/*.so.* \
	"
FILES_${PN}-dbg += " \
	${libdir}/snort_dynamicengine/.debug \
	${libdir}/snort_dynamicpreprocessor/.debug \
	${libdir}/snort_dynamicrules/.debug \
	"
FILES_${PN}-staticdev += " \
	${libdir}/snort_dynamicengine/*.a \
	${libdir}/snort_dynamicpreprocessor/*.a \
	${libdir}/snort_dynamicrules/*.a \
	${libdir}/snort/dynamic_preproc/*.a \
	${libdir}/snort/dynamic_output/*.a \
	"
FILES_${PN}-dev += " \
	${libdir}/snort_dynamicengine/*.la \
	${libdir}/snort_dynamicpreprocessor/*.la \
	${libdir}/snort_dynamicrules/*.la \
	${libdir}/snort_dynamicengine/*.so \
	${libdir}/snort_dynamicpreprocessor/*.so \
	${libdir}/snort_dynamicrules/*.so \
	${prefix}/src/snort_dynamicsrc \
	"
