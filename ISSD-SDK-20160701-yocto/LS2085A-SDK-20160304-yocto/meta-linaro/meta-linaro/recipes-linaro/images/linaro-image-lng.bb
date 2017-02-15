require linaro-image-common.inc

IMAGE_INSTALL += " \
    arndale-pre-boot \
    bridge-utils \
    calibrator \
    curl \
    cronie \
    fping \
    git \
    lmbench \
    lng-network-config \
    ltp \
    netperf \
    odp \
    openssh-sftp-server \
    openvswitch \
    packagegroup-core-buildessential \
    procps \
    python-numpy \
    qemu \
    rt-tests \
    trace-cmd \
    tunctl \
    usecpu \
    "

IMAGE_INSTALL_append_armv7a = " \
    latency-test \
    systemtap \
    valgrind \
    trinity-example \
    "

IMAGE_INSTALL_append_aarch64 = " \
    trinity-example \
    "

IMAGE_INSTALL_append_qemux86 = " \
    "

IMAGE_FEATURES += "\
    dev-pkgs \
    staticdev-pkgs \
    tools-debug \
    tools-sdk \
    "
IMAGE_FSTYPES_append_qemux86 += "cpio.gz"
IMAGE_FSTYPES_lng-x86-64 = "tar.gz cpio.gz"
IMAGE_FSTYPES_lng-rt-x86-64 = "tar.gz cpio.gz"

EXTRA_IMAGE_FEATURES_append_qemux86 = " autoserial"
FEATURE_PACKAGES_autoserial = "auto-serial-console"

IMAGE_PREPROCESS_COMMAND_qemux86 += "qemux86_fixup;"

qemux86_fixup() {
        # Since we use autoserial, remove serial consoles
        # See sysvinit-inittab recipe
        sed -i '/2345:respawn:\/sbin\/getty/d' ${IMAGE_ROOTFS}/etc/inittab

        # Add a default network interface
        echo "auto eth0" >> ${IMAGE_ROOTFS}/etc/network/interfaces
        echo "iface eth0 inet dhcp" >> ${IMAGE_ROOTFS}/etc/network/interfaces

        # The hostname can be changed by using
        # hostname_pn-base-files = "linaro"
        # See base-files recipe
        echo "linaro" > ${IMAGE_ROOTFS}${sysconfdir}/hostname
}
