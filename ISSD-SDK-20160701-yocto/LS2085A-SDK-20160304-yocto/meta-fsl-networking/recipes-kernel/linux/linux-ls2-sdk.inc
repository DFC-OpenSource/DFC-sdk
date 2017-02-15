DESCRIPTION = "Linux kernel for Freescale layerscape platforms"
SECTION = "kernel"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=d7810fab7487fb0aad327b76f1be7cd7"

inherit kernel siteinfo
require recipes-kernel/linux/linux-dtb.inc

SCMVERSION ?= "y"

DEPENDS_append = " libgcc"

KERNEL_CC_append = " ${TOOLCHAIN_OPTIONS}"
KERNEL_LD_append = " ${TOOLCHAIN_OPTIONS}"

S = "${WORKDIR}/git"
DELTA_KERNEL_DEFCONFIG = "freescale.config "
do_configure_prepend() {
    # copy desired defconfig so we pick it up for the real kernel_do_configure
    cp ${KERNEL_DEFCONFIG} ${B}/.config

    # check if bigendian is enabled
    if [ "${SITEINFO_ENDIANNESS}" = "be" ]; then
        echo "CONFIG_CPU_BIG_ENDIAN=y" >> ${B}/.config
    fi

    # add config fragments
    for deltacfg in ${DELTA_KERNEL_DEFCONFIG}; do
        if [ -f "${deltacfg}" ]; then
            ${S}/scripts/kconfig/merge_config.sh -m .config ${deltacfg}
        elif [ -f "${WORKDIR}/${deltacfg}" ]; then
            ${S}/scripts/kconfig/merge_config.sh -m .config ${WORKDIR}/${deltacfg}
        elif [ -f "${S}/arch/arm64/configs/${deltacfg}" ]; then
            ${S}/scripts/kconfig/merge_config.sh -m .config \
                ${S}/arch/arm64/configs/${deltacfg}
        fi
    done

    #add git revision to the local version
    if [ "${SCMVERSION}" = "y" ]; then
        # append sdk version if SDK_VERSION is defined
        sdkversion=''
        if [ -n "${SDK_VERSION}" ]; then
            sdkversion="-${SDK_VERSION}"
        fi
        head=`git --git-dir=${S}/.git rev-parse --verify --short HEAD 2> /dev/null`
        printf "%s%s%s" $sdkversion +g $head > ${B}/.scmversion
    fi
}

do_deploy_append() {
    rm -f ${KERNEL_OUTPUT}.gz
    gzip ${KERNEL_OUTPUT}
    install -m 0644 ${KERNEL_OUTPUT}.gz ${DEPLOYDIR}/${KERNEL_IMAGE_BASE_NAME}.gz
    ln -sf ${KERNEL_IMAGE_BASE_NAME}.gz ${DEPLOYDIR}/${KERNEL_IMAGETYPE}.gz
}

COMPATIBLE_MACHINE = "(ls2085aqds|ls2085aqds-be|ls2085ardb|ls2085ardb-be|ls2080ardb|ls2080aqds|ls2085aissd|ls2085aissd-be)"
