SUMMARY = "LiSt Open Files tool"
DESCRIPTION = "Lsof is a Unix-specific diagnostic tool. \
Its name stands for LiSt Open Files, and it does just that."
SECTION = "devel"
LICENSE = "BSD"

SRC_URI = "ftp://lsof.itap.purdue.edu/pub/tools/unix/lsof/lsof_${PV}.tar.bz2"

SRC_URI[md5sum] = "1b29c10db4aa88afcaeeaabeef6790db"
SRC_URI[sha256sum] = "fe6f9b0e26b779ccd0ea5a0b6327c2b5c38d207a6db16f61ac01bd6c44e5c99b"

LOCALSRC = "file://${WORKDIR}/lsof_${PV}/lsof_${PV}_src.tar"
S = "${WORKDIR}/lsof_${PV}_src"

LIC_FILES_CHKSUM = "file://${S}/00README;beginline=645;endline=679;md5=964df275d26429ba3b39dbb9f205172a"

python do_unpack () {
    # temporarily change S for unpack
    # of lsof_${PV}
    s = d.getVar('S')
    d.setVar('S', '${WORKDIR}/lsof_${PV}')
    bb.build.exec_func('base_do_unpack', d)
    # temporarily change SRC_URI for unpack
    # of lsof_${PV}_src
    src_uri = d.getVar('SRC_URI')
    d.setVar('SRC_URI', '${LOCALSRC}')
    d.setVar('S', s)
    bb.build.exec_func('base_do_unpack', d)
    d.setVar('SRC_URI', src_uri)
}

export LSOF_OS = "${TARGET_OS}"
LSOF_OS_libc-uclibc = "linux"
LSOF_OS_libc-glibc = "linux"
export LSOF_INCLUDE = "${STAGING_INCDIR}"

do_configure () {
	export LSOF_AR="${AR} cr"
	export LSOF_RANLIB="${RANLIB}"
        if [ "x${GLIBCVERSION}" != "x" ];then
                LINUX_CLIB=`echo ${GLIBCVERSION} |sed -e 's,\.,,g'`
                LINUX_CLIB="-DGLIBCV=${LINUX_CLIB}"
                export LINUX_CLIB
        fi
	yes | ./Configure ${LSOF_OS}
}

export I = "${STAGING_INCDIR}"
export L = "${STAGING_INCDIR}"
export EXTRA_OEMAKE = ""

do_compile () {
	oe_runmake 'CC=${CC}' 'CFGL=${LDFLAGS} -L./lib -llsof' 'DEBUG=' 'INCL=${CFLAGS}'
}

do_install () {
	install -d ${D}${sbindir} ${D}${mandir}/man8
	install -m 4755 lsof ${D}${sbindir}/lsof
	install -m 0644 lsof.8 ${D}${mandir}/man8/lsof.8
}
