rootfs_add_sdk_version () {
    if [ -n "${SDK_VERSION}" ]; then
        echo "${SDK_VERSION}" > ${IMAGE_ROOTFS}/etc/sdk-version
    fi
}

rootfs_delete_Image () {
    find ${IMAGE_ROOTFS} -name Image* | xargs rm -rf
}

rootfs_copy_core_image() {
    mkdir -p ${IMAGE_ROOTFS}/boot
    cp ${DEPLOY_DIR_IMAGE}/fsl-image-core-${MACHINE}.ext2.gz ${IMAGE_ROOTFS}/boot
}
