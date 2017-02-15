inherit core-image

IMAGE_INSTALL = " \
    packagegroup-core-standalone-hhvm-sdk-target \
    "

IMAGE_FEATURES += "\
    dev-pkgs \
    staticdev-pkgs \
    "

IMAGE_PREPROCESS_COMMAND += "do_delete_not_needed_dirs; "

fakeroot do_delete_not_needed_dirs () {
    for dir in bin boot dev etc home media mnt opt proc run sbin sys tmp var;
    do
        rm -rf ${IMAGE_ROOTFS}/${dir}
    done

    for dir in aarch64-oe-linux bin games sbin share src
    do
        rm -rf ${IMAGE_ROOTFS}/usr/${dir}
    done
}
