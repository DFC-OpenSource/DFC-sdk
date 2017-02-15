require images/fsl-image-flash.bb

# common opensource packages
IMAGE_INSTALL += " \
    gdbserver \
    packagegroup-fsl-core \
    packagegroup-fsl-dpaa2 \
"

ROOTFS_POSTPROCESS_COMMAND += "rootfs_delete_Image; "
