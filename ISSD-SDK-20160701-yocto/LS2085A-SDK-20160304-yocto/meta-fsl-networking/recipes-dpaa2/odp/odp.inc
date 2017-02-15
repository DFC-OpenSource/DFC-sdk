require odp-common.inc

PV = "1.4"

DEPENDS = "openssl"
RDEPENDS_${PN} = "bash odp-module libssl libcrypto"

inherit autotools-brokensep siteinfo

EXTRA_OECONF = '--with-platform=linux-dpaa2 \
                --with-sdk-install-path="${STAGING_DIR_TARGET}"' 

EXTRA_OEMAKE = 'CROSS_COMPILE="${TARGET_PREFIX}" SYSROOT="${STAGING_DIR_TARGET}"'
EXTRA_OECONF_append_ls2085ardb = " --enable-build-type=ls2085"
EXTRA_OECONF_append_ls2085aqds = " --enable-build-type=ls2085"
EXTRA_OECONF_append_ls2080ardb = " --enable-build-type=ls2080"
EXTRA_OECONF_append_ls2080aqds = " --enable-build-type=ls2080"

export ODP_ARCH = "${@d.getVar('TUNE_ARCH', True).replace('_${SITEINFO_ENDIANNESS}', '')}"

do_configure_prepend () {
    ./bootstrap
}

do_compile_prepend () {
    export ARCH="${ODP_ARCH}"
}
