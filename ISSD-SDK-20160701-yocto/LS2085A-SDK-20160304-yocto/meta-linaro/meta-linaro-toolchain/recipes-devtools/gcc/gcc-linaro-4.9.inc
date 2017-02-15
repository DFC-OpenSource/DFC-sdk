require recipes-devtools/gcc/gcc-4.9.inc
PV = "linaro-${BASEPV}"

MMYY = "15.03"
RELEASE = "20${MMYY}"
PR = "r${RELEASE}"
BINV = "4.9.3"

FILESPATH = "${@base_set_filespath([ '${FILE_DIRNAME}/gcc-${PV}' ], d)}"

SRC_URI = "https://releases.linaro.org/${MMYY}/components/toolchain/gcc-linaro/${BASEPV}/gcc-${PV}-${RELEASE}.tar.xz \
           file://0001-gcc-4.3.1-ARCH_FLAGS_FOR_TARGET.patch \
           file://0002-uclibc-conf.patch \
           file://0003-gcc-uclibc-locale-ctype_touplow_t.patch \
           file://0004-uclibc-locale.patch \
           file://0005-uclibc-locale-no__x.patch \
           file://0006-uclibc-locale-wchar_fix.patch \
           file://0007-uclibc-locale-update.patch \
           file://0008-missing-execinfo_h.patch \
           file://0009-c99-snprintf.patch \
           file://0010-c99-complex-ugly-hack.patch \
           file://0011-index_macro.patch \
           file://0013-libstdc-namespace.patch \
           file://0014-sh-pr24836.patch \
           file://0015-arm-Use-TARGET_ENDIAN_OPTION-for-determining-MULTILI.patch \
           file://0016-gcc-poison-system-directories.patch \
           file://0017-gcc-poison-dir-extend.patch \
           file://0018-gcc-4.3.3-SYSROOT_CFLAGS_FOR_TARGET.patch \
           file://0019-64-bit-multilib-hack.patch \
           file://0020-optional-libstdc.patch \
           file://0021-gcc-disable-MASK_RELAX_PIC_CALLS-bit.patch \
           file://0022-COLLECT_GCC_OPTIONS.patch \
           file://0023-Use-the-defaults.h-in-B-instead-of-S-and-t-oe-in-B.patch \
           file://0024-PR-target-32219.patch \
           file://0025-fortran-cross-compile-hack.patch \
           file://0026-libgcc-sjlj-check.patch \
           file://0027-cpp-honor-sysroot.patch \
           file://0028-MIPS64-Default-to-N64-ABI.patch \
           file://0029-Define-GLIBC_DYNAMIC_LINKER-and-UCLIBC_DYNAMIC_LINKE.patch \
           file://0030-gcc-Fix-argument-list-too-long-error.patch \
           file://0031-Disable-sdt.patch \
           file://0032-libtool.patch \
           file://0033-gcc-armv4-pass-fix-v4bx-to-linker-to-support-EABI.patch \
           file://0034-Use-the-multilib-config-files-from-B-instead-of-usin.patch \
           file://0040-fix-g++-sysroot.patch \
           file://0041-libtool-avoid-libdir.patch \
           file://0043-cpp.patch \
           file://0044-gengtypes.patch \
           file://0046-libatomic-deptracking.patch \
           file://0047-repomembug.patch \
           file://0049-Enable-SPE-AltiVec-generation-on-powepc-linux-target.patch \
           file://0050-Revert-Use-dbx_reg_number-for-spanning-registers.patch \
           file://0051-eabispe.patch \
           file://0052-Add-target-hook-to-override-DWARF2-frame-register-si.patch \
           file://0053-gcc-fix-segfault-from-calling-free-on-non-malloc-d-a.patch \
           file://0054-gcc-Makefile.in-fix-parallel-building-failure.patch \
           file://0055-dwarf-reg-processing-helper.patch \
           file://0056-define-default-cfa-register-mapping.patch \
           file://0057-aarch64-config.patch;apply=no \
           file://0058-gcc-r212171.patch \
           file://0059-gcc-PR-rtl-optimization-63348.patch \
           file://0060-Only-allow-e500-double-in-SPE_SIMD_REGNO_P-registers.patch \
           file://0061-target-gcc-includedir.patch \
           file://AArch64-Define-BE-loader-name-default-be.patch \
           file://use-lib-for-aarch64.patch \
           file://Fix-large-integer-type-issue-for-aarch64-small-model.patch \
          "

SRC_URI[md5sum] = "f9d256d120adfbb45dd3e2d22b70cba9"
SRC_URI[sha256sum] = "1338d962f90d51806f8e5844c96c8f746dbc5c9a8066e0fe10ba0b21f5401611"

S = "${TMPDIR}/work-shared/gcc-${PV}-${PR}/gcc-${PV}-${RELEASE}"
B = "${WORKDIR}/gcc-${PV}-${RELEASE}/build.${HOST_SYS}.${TARGET_SYS}"

EXTRA_OECONF_append_aarchilp32 = " --with-abi=ilp32"
