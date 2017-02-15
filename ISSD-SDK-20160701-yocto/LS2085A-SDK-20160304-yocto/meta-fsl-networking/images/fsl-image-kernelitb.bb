#
# Copyright(C) 2014 Freescale
#
DESCRIPTION = "A FIT image comprising the Linux image, dtb and rootfs image"
LICENSE = "MIT"

SRC_URI = "file://${KERNEL_ITS_FILE}"

S = "${WORKDIR}"

inherit deploy siteinfo

do_patch[noexec] = "1"
do_configure[noexec] = "1"
do_compile[noexec] = "1"
do_install[noexec] = "1"
do_populate_sysroot[noexec] = "1"
do_package[noexec] = "1"
do_packagedata[noexec] = "1"
do_package_write_ipk[noexec] = "1"
do_package_write_deb[noexec] = "1"
do_package_write_rpm[noexec] = "1"

KERNEL_IMAGE ?= "${KERNEL_IMAGETYPE}"
KERNEL_DEVICETREE ?= "freescale/fsl-ls2085a-rdb.dtb"
DTB_IMAGE ?= "${KERNEL_IMAGETYPE}-`basename ${KERNEL_DEVICETREE}`"
ROOTFS_IMAGE ?= "fsl-image-minimal"
KERNEL_ITB_IMAGE ?= "kernel.itb"
PACKAGE_ARCH = "${MACHINE_ARCH}"
MACHINE_BASE = "${@d.getVar('MACHINE', True).replace('-${SITEINFO_ENDIANNESS}', '')}"

do_fetch[nostamp] = "1"
do_unpack[nostamp] = "1"
do_deploy[nostamp] = "1"
do_deploy[depends] += "u-boot-mkimage-native:do_build virtual/kernel:do_build ${ROOTFS_IMAGE}:do_build"

do_deploy () {
    sed -i -e "s,./arch/arm64/boot/Image.gz,${KERNEL_IMAGE}.gz," ${S}/${KERNEL_ITS_FILE}
    sed -i -e "s,./arch/arm64/boot/dts/${KERNEL_DEVICETREE},${DTB_IMAGE}," ${S}/${KERNEL_ITS_FILE}
    sed -i -e "s,./fsl-image-${MACHINE_BASE}.ext2.gz,${ROOTFS_IMAGE}-${MACHINE}.ext2.gz," ${S}/${KERNEL_ITS_FILE}

    install -m 644 ${DEPLOY_DIR_IMAGE}/${KERNEL_IMAGE} ${S}/
    rm -f ${S}/${KERNEL_IMAGE}.gz
    gzip ${S}/${KERNEL_IMAGE}
    install -m 644 ${DEPLOY_DIR_IMAGE}/${DTB_IMAGE} ${S}/
    install -m 644 ${DEPLOY_DIR_IMAGE}/${ROOTFS_IMAGE}-${MACHINE}.ext2.gz ${S}/

    mkimage -f ${KERNEL_ITS_FILE} kernel.itb

    install -d ${DEPLOYDIR}
    install -m 644 ${S}/kernel.itb ${DEPLOYDIR}/kernel-${MACHINE}-${DATETIME}.itb
    cd ${DEPLOYDIR}
    ln -sf kernel-${MACHINE}-${DATETIME}.itb kernel-${MACHINE}.itb
    cd -
}

addtask deploy before build
