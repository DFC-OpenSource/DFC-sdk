SUMMARY = "Utilities and libraries for handling compiled object files"
HOMEPAGE = "https://fedorahosted.org/elfutils"
SECTION = "base"
LICENSE = "(GPLv3 & Elfutils-Exception)"
LIC_FILES_CHKSUM = "file://COPYING;md5=d32239bcb673463ab874e80d47fae504"
DEPENDS = "libtool bzip2 zlib virtual/libintl"

SRC_URI = "https://fedorahosted.org/releases/e/l/elfutils/${PV}/elfutils-${PV}.tar.bz2"

SRC_URI[md5sum] = "e1b9847c9a6a1ad340de8d47a863ec52"
SRC_URI[sha256sum] = "570c91a1783fa5386aaa2dfdd08dda1de777c2b63bf3b9c1437d635ffdd7a070"

SRC_URI += "\
        file://mempcpy.patch \
        file://dso-link-change.patch \
        file://Fix_elf_cvt_gunhash.patch \
        file://fixheadercheck.patch \
        file://0001-elf_getarsym-Silence-Werror-maybe-uninitialized-fals.patch \
        file://0001-libelf-Fix-dir-traversal-vuln-in-ar-extraction.patch \
"

# pick the patch from debian
# http://ftp.de.debian.org/debian/pool/main/e/elfutils/elfutils_0.159-4.debian.tar.xz
SRC_URI += "\
        file://redhat-portability.diff \
        file://hppa_backend.diff \
        file://arm_backend.diff \
        file://mips_backend.diff \
        file://m68k_backend.diff \
        file://testsuite-ignore-elflint.diff \
        file://scanf-format.patch \
        file://mips_readelf_w.patch \
        file://arm_func_value.patch \
        file://arm_unwind_ret_mask.patch \
        file://non_linux.patch \
"

# Only apply when building uclibc based target recipe
SRC_URI_append_libc-uclibc = " file://uclibc-support-for-elfutils-0.161.patch"

# The buildsystem wants to generate 2 .h files from source using a binary it just built,
# which can not pass the cross compiling, so let's work around it by adding 2 .h files
# along with the do_configure_prepend()

SRC_URI += "\
        file://i386_dis.h \
        file://x86_64_dis.h \
"
inherit autotools gettext

EXTRA_OECONF = "--program-prefix=eu- --without-lzma"
EXTRA_OECONF_append_class-native = " --without-bzlib"
EXTRA_OECONF_append_libc-uclibc = " --enable-uclibc"

do_configure_prepend() {
	sed -i '/^i386_dis.h:/,+4 {/.*/d}' ${S}/libcpu/Makefile.am

	cp ${WORKDIR}/*dis.h ${S}/libcpu
}

do_install_append() {
	if [ "${TARGET_ARCH}" != "x86_64" ] && [ -z `echo "${TARGET_ARCH}"|grep 'i.86'` ];then
		rm ${D}${bindir}/eu-objdump
	fi
}

# we can not build complete elfutils when using uclibc
# but some recipes e.g. gcc 4.5 depends on libelf so we
# build only libelf for uclibc case

EXTRA_OEMAKE_libc-uclibc = "-C libelf"
EXTRA_OEMAKE_class-native = ""
EXTRA_OEMAKE_class-nativesdk = ""

BBCLASSEXTEND = "native nativesdk"

# Package utilities separately
PACKAGES =+ "${PN}-binutils libelf libasm libdw libdw-dev libasm-dev libelf-dev"
FILES_${PN}-binutils = "\
    ${bindir}/eu-addr2line \
    ${bindir}/eu-ld \
    ${bindir}/eu-nm \
    ${bindir}/eu-readelf \
    ${bindir}/eu-size \
    ${bindir}/eu-strip"

FILES_libelf = "${libdir}/libelf-${PV}.so ${libdir}/libelf.so.*"
FILES_libasm = "${libdir}/libasm-${PV}.so ${libdir}/libasm.so.*"
FILES_libdw  = "${libdir}/libdw-${PV}.so ${libdir}/libdw.so.* ${libdir}/elfutils/lib*"
FILES_libelf-dev = "${libdir}/libelf.so ${includedir}"
FILES_libasm-dev = "${libdir}/libasm.so ${includedir}/elfutils/libasm.h"
FILES_libdw-dev  = "${libdir}/libdw.so  ${includedir}/dwarf.h ${includedir}/elfutils/libdw*.h"
# Some packages have the version preceeding the .so instead properly
# versioned .so.<version>, so we need to reorder and repackage.
#FILES_${PN} += "${libdir}/*-${PV}.so ${base_libdir}/*-${PV}.so"
#FILES_SOLIBSDEV = "${libdir}/libasm.so ${libdir}/libdw.so ${libdir}/libelf.so"

# The package contains symlinks that trip up insane
INSANE_SKIP_${MLPREFIX}libdw = "dev-so"
