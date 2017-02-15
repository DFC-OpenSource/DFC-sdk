KORG_ARCHIVE_COMPRESSION = "xz"

require recipes-kernel/linux-libc-headers/linux-libc-headers.inc

SRC_URI_append_aarchilp32 = " \
                             file://0001-ARM64-Force-LP64-to-compile-the-kernel.patch \
                             file://0002-ARM64-Rename-COMPAT-to-AARCH32_EL0-in-Kconfig.patch \
                             file://0003-ARM64-Change-some-CONFIG_COMPAT-over-to-use-CONFIG_A.patch \
                             file://0004-ARM64-ILP32-Set-kernel_long-to-long-long-so-we-can-r.patch \
                             file://0005-ARM64-UAPI-Set-the-correct-__BITS_PER_LONG-for-ILP32.patch \
                             file://0006-Allow-for-some-signal-structures-to-be-the-same-betw.patch \
                             file://0007-ARM64-ILP32-Use-the-same-size-and-layout-of-the-sign.patch \
                             file://0008-Allow-a-32bit-ABI-to-use-the-naming-of-the-64bit-ABI.patch \
                             file://0009-ARM64-ILP32-Use-the-same-syscall-names-as-LP64.patch \
                             file://0010-ARM64-Introduce-is_a32_task-is_a32_thread-and-TIF_AA.patch \
                             file://0011-ARM64-Add-is_ilp32_compat_task-and-is_ilp32_compat_t.patch \
                             file://0012-ARM64-ILP32-COMPAT_USE_64BIT_TIME-is-true-for-ILP32-.patch \
                             file://0013-ARM64-ILP32-Use-the-non-compat-HWCAP-for-ILP32.patch \
                             file://0014-ARM64-ILP32-use-the-standard-start_thread-for-ILP32-.patch \
                             file://0015-compat_binfmt_elf-coredump-Allow-some-core-dump-macr.patch \
                             file://0016-ARM64-ILP32-Support-core-dump-for-ILP32.patch \
                             file://0017-ARM64-Add-loading-of-ILP32-binaries.patch \
                             file://0018-ARM64-Add-vdso-for-ILP32-and-use-it-for-the-signal-r.patch \
                             file://0019-ptrace-Allow-compat-to-use-the-native-siginfo.patch \
                             file://0020-ARM64-ILP32-The-native-siginfo-is-used-instead-of-th.patch \
                             file://0021-ARM64-ILP32-Use-a-seperate-syscall-table-as-a-few-sy.patch \
                             file://0022-ARM64-ILP32-Fix-signal-return-for-ILP32-when-the-use.patch \
                             file://0023-ARM64-Add-ARM64_ILP32-to-Kconfig.patch \
                             file://0024-Add-documentation-about-ARM64-ILP32-ABI.patch \
                             file://0025-ARM64-ILP32-fix-the-compilation-error-due-to-mistype.patch \
                            "                              

SRC_URI[md5sum] = "9e854df51ca3fef8bfe566dbd7b89241"
SRC_URI[sha256sum] = "becc413cc9e6d7f5cc52a3ce66d65c3725bc1d1cc1001f4ce6c32b69eb188cbd"

