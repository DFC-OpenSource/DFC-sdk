require linaro-image-alip.bb

IMAGE_FEATURES += "\
    dev-pkgs \
    "

IMAGE_PREPROCESS_COMMAND += "do_delete_not_needed_dirs; "

fakeroot do_delete_not_needed_dirs () {
    for dir in bin boot dev etc home media mnt opt proc run sbin sys tmp var;
    do
        rm -rf ${IMAGE_ROOTFS}/${dir}
    done

    for dir in arm-oe-linux-gnueabi aarch64-oe-linux bin games sbin share src
    do
        rm -rf ${IMAGE_ROOTFS}/usr/${dir}
    done
}
