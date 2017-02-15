require recipes-core/glibc/glibc.inc

DEPENDS += "gperf-native kconfig-frontends-native"

MMYY = "14.11"
RELEASE = "20${MMYY}"
PR = "r${RELEASE}"

SRC_URI = "http://releases.linaro.org/${MMYY}/components/toolchain/glibc-linaro/glibc-${PV}-${RELEASE}.tar.xz \
           file://etc/ld.so.conf \
           file://generate-supported.mk \
           file://IO-acquire-lock-fix.patch \
           file://mips-rld-map-check.patch \
           file://etc/ld.so.conf \
           file://generate-supported.mk \
           file://glibc.fix_sqrt2.patch \
           file://multilib_readlib.patch \
           file://ppc-sqrt_finite.patch \
           file://ppc_slow_ieee754_sqrt.patch \
           file://add_resource_h_to_wait_h.patch \
           file://fsl-ppc-no-fsqrt.patch \
           file://0001-R_ARM_TLS_DTPOFF32.patch \
           file://0001-eglibc-run-libm-err-tab.pl-with-specific-dirs-in-S.patch \
           file://fix-tibetian-locales.patch \
           file://ppce6500-32b_slow_ieee754_sqrt.patch \
           file://grok_gold.patch \
           file://fix_am_rootsbindir.patch \
           ${EGLIBCPATCHES} \
           ${ILP32PATCHES} \
          "
EGLIBCPATCHES = "\
           file://timezone-re-written-tzselect-as-posix-sh.patch \
           file://eglibc.patch \
           file://option-groups.patch \
           file://GLRO_dl_debug_mask.patch \
           file://eglibc-header-bootstrap.patch \
           file://eglibc-resolv-dynamic.patch \
           file://eglibc-ppc8xx-cache-line-workaround.patch \
           file://eglibc-sh4-fpscr_values.patch \
           file://eglibc-use-option-groups.patch \
          "
ILP32PATCHES = " \
           file://0002-Fix-utmp-struct-for-compatibility-reasons.patch \
           file://0003-Allow-sigset-be-an-array-of-a-different-type.patch \
           file://0004-Add-ability-for-the-IPC-structures-msqid_ds-semid_ds.patch \
           file://0005-Allow-rusage-work-on-a-big-endian-32bit-on-64bit-tar.patch \
           file://0006-Allow-fd_mask-type-not-be-an-array-of-long.patch \
           file://0007-Allow-some-fields-of-siginfo-to-be-different-from-th.patch \
           file://0008-Allow-generic-stat-and-statfs-not-have-padding-for-3.patch \
           file://0009-Add-header-guards-to-sysdep.h-headers.patch \
           file://0010-Add-dynamic-ILP32-AARCH64-relocations-to-elf.h.patch \
           file://0011-Add-PTR_REG-PTR_LOG_SIZE-and-PTR_SIZE.-Use-it-in-LDS.patch \
           file://0012-Use-PTR_REG-in-crti.S.patch \
           file://0013-Use-PTR_REG-PTR_SIZE-PTR_SIZE_LOG-in-dl-tlsesc.S.patch \
           file://0014-Use-PTR_-macros-in-dl-trampoline.S.patch \
           file://0015-Use-PTR_-in-start.S.patch \
           file://0016-Use-PTR_REG-in-getcontext.S.patch \
           file://0017-Detect-ILP32-in-configure-scripts.patch \
           file://0018-Syscalls-for-ILP32-are-passed-always-via-64bit-value.patch \
           file://0019-Reformat-inline-asm-in-elf_machine_load_address.patch \
           file://0020-Add-ILP32-support-to-elf_machine_load_address.patch \
           file://0021-Set-up-wordsize-for-ILP32.patch \
           file://0022-Add-ILP32-to-makefiles.patch \
           file://0023-Add-support-to-ldconfig-for-ILP32-and-libilp32.patch \
           file://0024-Add-ILP32-ld.so-to-the-known-interpreter-names.patch \
           file://0025-Add-ldd-rewrite.sed-so-that-ilp32-ld.so-can-be-found.patch \
           file://0026-Add-kernel_sigaction.h-for-AARCH64-ILP32.patch \
           file://0027-Add-sigstack.h-header-for-ILP32-reasons.patch \
           file://0028-Fix-up-ucontext-for-ILP32.patch \
           file://0029-Add-typesizes.h-for-ILP32.patch \
           file://0030-Make-lp64-and-ilp32-directories.patch \
           file://0031-sysdeps-unix-sysv-linux-aarch64-sysdep.h-Fix-crash-i.patch \
           file://0032-sysdeps-unix-sysv-linux-aarch64-configure-Reduce-ker.patch \
          "
SRC_URI[md5sum] = "ca2035b47d86856ffdd201ce2a12e60e"
SRC_URI[sha256sum] = "2db756f9f9281f78443cd04fc544fc2d8bce38037d5f75c8284bd8b0ccf9c9ed"

LIC_FILES_CHKSUM = "file://LICENSES;md5=e9a558e243b36d3209f380deb394b213 \
      file://COPYING;md5=b234ee4d69f5fce4486a80fdaf4a4263 \
      file://posix/rxspencer/COPYRIGHT;md5=dc5485bb394a13b2332ec1c785f5d83a \
      file://COPYING.LIB;md5=4fbd65380cdd255951079008b364516c"

SRC_URI_append_class-nativesdk = " file://ld-search-order.patch \
            file://relocatable_sdk.patch \
            file://relocatable_sdk_fix_openpath.patch \
            "
S = "${WORKDIR}/glibc-${PV}-${RELEASE}"
B = "${WORKDIR}/build-${TARGET_SYS}"

PACKAGES_DYNAMIC = ""

# the -isystem in bitbake.conf screws up glibc do_stage
BUILD_CPPFLAGS = "-I${STAGING_INCDIR_NATIVE}"
TARGET_CPPFLAGS = "-I${STAGING_DIR_TARGET}${includedir}"

GLIBC_BROKEN_LOCALES = " _ER _ET so_ET yn_ER sid_ET tr_TR mn_MN gez_ET gez_ER bn_BD te_IN es_CR.ISO-8859-1"

FILESPATH = "${@base_set_filespath([ '${FILE_DIRNAME}/glibc-${PV}', '${FILE_DIRNAME}/glibc', '${FILE_DIRNAME}/files', '${FILE_DIRNAME}' ], d)}"

#
# We will skip parsing glibc when system C library selection is not glibc
# this helps in easing out parsing for non-glibc system libraries
#

python __anonymous () {
    import re
    notglibc = (re.match('.*uclibc$', d.getVar('TARGET_OS', True)) != None) or (re.match('.*musl$', d.getVar('TARGET_OS', True)) != None)
    if notglibc:
        raise bb.parse.SkipPackage("incompatible with target %s" %
                                   d.getVar('TARGET_OS', True))
}

export libc_cv_slibdir = "${base_libdir}"

EXTRA_OECONF = "--enable-kernel=${OLDEST_KERNEL} \
                --without-cvs --disable-profile \
                --disable-debug --without-gd \
                --enable-clocale=gnu \
                --enable-add-ons \
                --with-headers=${STAGING_INCDIR} \
                --without-selinux \
                --enable-obsolete-rpc \
                --with-kconfig=${STAGING_BINDIR_NATIVE} \
                ${GLIBC_EXTRA_OECONF}"

EXTRA_OECONF += "${@get_libc_fpu_setting(bb, d)}"

do_patch_append() {
    bb.build.exec_func('do_fix_readlib_c', d)
}

# for mips glibc now builds syscall tables for all abi's
# so we make sure that we choose right march option which is
# compatible with o32,n32 and n64 abi's
# e.g. -march=mips32 is not compatible with n32 and n64 therefore
# we filter it out in such case -march=from-abi which will be
# mips1 when using o32 and mips3 when using n32/n64

TUNE_CCARGS_mips := "${@oe_filter_out('-march=mips32', '${TUNE_CCARGS}', d)}"
TUNE_CCARGS_mipsel := "${@oe_filter_out('-march=mips32', '${TUNE_CCARGS}', d)}"

do_fix_readlib_c () {
	sed -i -e 's#OECORE_KNOWN_INTERPRETER_NAMES#${EGLIBC_KNOWN_INTERPRETER_NAMES}#' ${S}/elf/readlib.c
}

do_configure () {
# override this function to avoid the autoconf/automake/aclocal/autoheader
# calls for now
# don't pass CPPFLAGS into configure, since it upsets the kernel-headers
# version check and doesn't really help with anything
        if [ -z "`which rpcgen`" ]; then
                echo "rpcgen not found.  Install glibc-devel."
                exit 1
        fi
        (cd ${S} && gnu-configize) || die "failure in running gnu-configize"
        find ${S} -name "configure" | xargs touch
        CPPFLAGS="" oe_runconf
}

rpcsvc = "bootparam_prot.x nlm_prot.x rstat.x \
	  yppasswd.x klm_prot.x rex.x sm_inter.x mount.x \
	  rusers.x spray.x nfs_prot.x rquota.x key_prot.x"

do_compile () {
	# -Wl,-rpath-link <staging>/lib in LDFLAGS can cause breakage if another glibc is in staging
	unset LDFLAGS
	base_do_compile
	(
		cd ${S}/sunrpc/rpcsvc
		for r in ${rpcsvc}; do
			h=`echo $r|sed -e's,\.x$,.h,'`
			rpcgen -h $r -o $h || bbwarn "unable to generate header for $r"
		done
	)
	echo "Adjust ldd script"
	if [ -n "${RTLDLIST}" ]
	then
		prevrtld=`cat ${B}/elf/ldd | grep "^RTLDLIST=" | sed 's#^RTLDLIST="\?\([^"]*\)"\?$#\1#'`
		if [ "${prevrtld}" != "${RTLDLIST}" ]
		then
			sed -i ${B}/elf/ldd -e "s#^RTLDLIST=.*\$#RTLDLIST=\"${prevrtld} ${RTLDLIST}\"#"
		fi
	fi

}

# In case of aarch64_be install symlink to ld-linux-aarch64_be.so.1
# to enable transition of toolchain and executables that are not yet
# aware about aarch64_be run-time linker name change.
#
# Currently there is no use case that requires both LE and BE glibc
# installed into the same rootfs, so our transitional symlink should
# be fine.
#
do_install_append_aarch64-be() {
    if [ -e ${D}${base_libdir}/ld-linux-aarch64_be.so.1 ] ; then
        ln -sf ld-linux-aarch64_be.so.1 ${D}${base_libdir}/ld-linux-aarch64.so.1
    fi
}

require recipes-core/glibc/glibc-package.inc

BBCLASSEXTEND = "nativesdk"
