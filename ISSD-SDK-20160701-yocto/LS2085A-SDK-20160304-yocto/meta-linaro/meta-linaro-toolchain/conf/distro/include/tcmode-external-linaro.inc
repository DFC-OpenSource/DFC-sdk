#
# Configuration to use an external Linaro binary toolchain
#

EXTERNAL_TOOLCHAIN ?= "/usr/local/linaro-binary-toolchain/${TARGET_ARCH}"

TOOLCHAIN_PATH_ADD = "${EXTERNAL_TOOLCHAIN}/bin:"
PATH =. "${TOOLCHAIN_PATH_ADD}"

ELT_TARGET_SYS_arm ?= "arm-linux-gnueabihf"
ELT_TARGET_SYS_aarch64 ?= "aarch64-linux-gnu"
ELT_TARGET_SYS = "${TARGET_SYS}"
TARGET_PREFIX = "${ELT_TARGET_SYS}-"

GCCMULTILIB_forcevariable = "--disable-multilib"
IMAGE_LINGUAS_forcevariable = ""

PREFERRED_PROVIDER_linux-libc-headers = "external-linaro-toolchain"
PREFERRED_PROVIDER_linux-libc-headers-dev = "external-linaro-toolchain"
PREFERRED_PROVIDER_virtual/${TARGET_PREFIX}gcc = "external-linaro-toolchain"
PREFERRED_PROVIDER_virtual/${TARGET_PREFIX}gcc-initial = "external-linaro-toolchain"
PREFERRED_PROVIDER_virtual/${TARGET_PREFIX}g++ = "external-linaro-toolchain"
PREFERRED_PROVIDER_virtual/${TARGET_PREFIX}binutils = "external-linaro-toolchain"
PREFERRED_PROVIDER_virtual/${TARGET_PREFIX}libc-for-gcc = "external-linaro-toolchain"
PREFERRED_PROVIDER_virtual/${TARGET_PREFIX}compilerlibs = "external-linaro-toolchain"
PREFERRED_PROVIDER_glibc = "external-linaro-toolchain"
PREFERRED_PROVIDER_libgcc = "external-linaro-toolchain"
PREFERRED_PROVIDER_virtual/libc = "external-linaro-toolchain"
PREFERRED_PROVIDER_virtual/libintl = "external-linaro-toolchain"
PREFERRED_PROVIDER_virtual/libiconv = "external-linaro-toolchain"
PREFERRED_PROVIDER_glibc-thread-db = "external-linaro-toolchain"
PREFERRED_PROVIDER_glibc-mtrace = "external-linaro-toolchain"
PREFERRED_PROVIDER_libc-mtrace = "external-linaro-toolchain"
PREFERRED_PROVIDER_virtual/linux-libc-headers = "external-linaro-toolchain"

TARGET_CPPFLAGS_prepend = " -isystem${EXTERNAL_TOOLCHAIN}/${ELT_TARGET_SYS}/include "
TARGET_LDFLAGS_prepend = " -L${EXTERNAL_TOOLCHAIN}/${ELT_TARGET_SYS}/lib -Wl,-rpath-link,${EXTERNAL_TOOLCHAIN}/${ELT_TARGET_SYS}/lib "

TOOLCHAIN_OPTIONS = " --sysroot=${STAGING_DIR_HOST}"

DISTRO_FEATURES_LIBC = "ipv4 ipv6 libc-backtrace libc-big-macros libc-bsd libc-cxx-tests libc-catgets libc-crypt \
			libc-crypt-ufc libc-db-aliases libc-envz libc-fcvt libc-fmtmsg libc-fstab libc-ftraverse \
			libc-getlogin libc-idn libc-inet-anl libc-libm libc-libm-big \
			libc-memusage libc-nis libc-nsswitch libc-rcmd libc-rtld-debug libc-spawn libc-streams libc-sunrpc \
			libc-utmp libc-utmpx libc-wordexp libc-posix-clang-wchar libc-posix-regexp libc-posix-regexp-glibc \
			libc-posix-wchar-io"

ENABLE_BINARY_LOCALE_GENERATION = "0"
GLIBC_INTERNAL_USE_BINARY_LOCALE = "precompiled"

ERROR_QA[type] ?= "list"
python toolchain_metadata_setup () {
    import subprocess
    if not isinstance(e, bb.event.ConfigParsed):
        return

    d = e.data
    l = d.createCopy()
    l.finalize()
    oe_import(l)

    external_toolchain = l.getVar('EXTERNAL_TOOLCHAIN', True)
    if not external_toolchain or external_toolchain == 'UNDEFINED':
        bb.fatal("Error: EXTERNAL_TOOLCHAIN must be set to the path to your linaro toolchain")

    if not os.path.exists(external_toolchain):
        bb.fatal("Error: EXTERNAL_TOOLCHAIN path '%s' does not exist" % external_toolchain)

    # The external toolchain may not have been built with the oe-core preferred
    # gnu hash setting, so ensure that the corresponding sanity check is not an error.
    error_qa = oe.data.typed_value('ERROR_QA', l)
    if 'ldflags' in error_qa:
        error_qa.remove('ldflags')
        d.setVar('ERROR_QA', ' '.join(error_qa))
}
addhandler toolchain_metadata_setup

def populate_toolchain_links(d):
    import errno
    import os
    from glob import glob

    d = d.createCopy()
    d.finalize()

    pattern = bb.data.expand('${EXTERNAL_TOOLCHAIN}/bin/${TARGET_PREFIX}*', d)
    files = glob(pattern)
    if not files:
        bb.fatal("Unable to populate toolchain binary symlinks")

    bindir = d.getVar('STAGING_BINDIR_TOOLCHAIN', True)
    bb.mkdirhier(bindir)
    for f in files:
        base = os.path.basename(f)
        newpath = os.path.join(bindir, base)
        try:
            os.symlink(f, newpath)
        except OSError as exc:
            if exc.errno == errno.EEXIST:
                break
            bb.fatal("Unable to populate toolchain binary symlink for %s: %s" % (newpath, exc))

require conf/distro/include/external-linaro-toolchain-versions.inc
