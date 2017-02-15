DESCRIPTION = "Layerscape Network Application Development Kit"

require nadk-common.inc

do_configure[depends] += "virtual/kernel:do_shared_workdir"
DEPENDS = "odp odp-headers"

inherit siteinfo

EXTRA_OEMAKE = 'CC="${CC}" LD="${LD}" AR="${AR}"'
EXTRA_OEMAKE_append_ls2085ardb = " BUILD_TYPE=LS2085"
EXTRA_OEMAKE_append_ls2085aqds = " BUILD_TYPE=LS2085"
EXTRA_OEMAKE_append_ls2080ardb = " BUILD_TYPE=LS2080"
EXTRA_OEMAKE_append_ls2080aqds = " BUILD_TYPE=LS2080"

export NADK_ARCH = "${@d.getVar('TUNE_ARCH', True).replace('_${SITEINFO_ENDIANNESS}', '')}"
export KERNEL_PATH = "${STAGING_KERNEL_DIR}"
export TARGET = "ldpaa"

do_compile_prepend () {
    export ARCH="${NADK_ARCH}"
}

FILES_${PN} += "/usr/nadk/*"
