require images/fsl-image-core.bb

# copy core rootfs image
ROOTFS_POSTPROCESS_COMMAND += "rootfs_copy_core_image;"
ROOTFS_POSTPROCESS_COMMAND_remove = "rootfs_delete_Image;"

IMAGE_INSTALL += " \
    docker \
    docker-registry \
    libvirt \
    libvirt-libvirtd \
    libvirt-virsh \
    lxc \
    qemu"

do_rootfs[depends] += "fsl-image-core:do_rootfs"
