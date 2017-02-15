require images/fsl-image-core.bb

PRIVATE_FULL = "yes"

# copy the manifest and the license text for each package to image
COPY_LIC_MANIFEST = "1"
COPY_LIC_DIRS = "1"

IMAGE_INSTALL += " \
    kernel-devicetree \
    packagegroup-core-buildessential \
    packagegroup-core-nfs-server \
    packagegroup-core-tools-debug \
    packagegroup-fsl-extend \
    u-boot-ls2 \
"

IMAGE_FSTYPES = "tar.gz"

ROOTFS_POSTPROCESS_COMMAND_remove = "rootfs_delete_Image;"
