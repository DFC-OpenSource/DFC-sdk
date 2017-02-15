SRC_URI_append= " git://git.kernel.org/pub/scm/linux/kernel/git/cmarinas/boot-wrapper-aarch64.git;protocol=http;branch=master;name=bootwrapper;destsuffix=bootwrapper \
"

SRCREV_bootwrapper = "26a17ad59544f026878efdb0524ce66b7a314784"
BW = "${WORKDIR}/bootwrapper"
DTBDIR="${B}/arch/arm64/boot/dts/"

# Bootwrapper needs libgcc_s.so
DEPENDS =+ "libgcc"

export WRAPCFLAGS = "${CFLAGS}"
export WRAPCPPFLAGS = "${CPPFLAGS}"
export WRAPCXXFLAGS = "${CXXFLAGS}"
export WRAPLDFLAGS = "${LDFLAGS}"

do_compile_append() {
    oe_runmake dtbs
    cd ${BW}

    # Ensure the compile can find libgcc.a and other userspace friends
    export CFLAGS="${WRAPCFLAGS}"
    export CPPFLAGS="${WRAPCPPFLAGS}"
    export CXXFLAGS="${WRAPCXXFLAGS}"
    export LDFLAGS="${WRAPLDFLAGS}"

    autoreconf -vfi
    dtb=`find ${DTBDIR} -type f -name "foundation-v8.dtb"`
    ./configure --host=aarch64-linux-gnu \
                --with-kernel-dir="${B}" \
                --with-cmdline="${BOOTARGS_COMMON} root=/dev/vda" \
                --with-dtb="${dtb}"
    make IMAGE=linux-system-foundation.axf clean
    make DTC=${B}/scripts/dtc/dtc \
         KERNEL_DTB="${dtb}" \
         IMAGE=linux-system-foundation.axf
    make IMAGE=img-foundation.axf clean
    ./configure --host=aarch64-linux-gnu \
                --with-kernel-dir="${B}" \
                --with-cmdline="${BOOTARGS_COMMON} root=/dev/vda2" \
                --with-dtb="${dtb}"
    make DTC=${B}/scripts/dtc/dtc \
         KERNEL_DTB="${dtb}" \
         IMAGE=img-foundation.axf
    make IMAGE=img.axf clean
    dtb=`find ${DTBDIR} -type f -name "rtsm_ve-aemv8a.dtb"`
    ./configure --host=aarch64-linux-gnu \
                --with-kernel-dir="${B}" \
                --with-cmdline="${BOOTARGS_COMMON} root=/dev/mmcblk0p2" \
                --with-dtb="${dtb}"
    make DTC=${B}/scripts/dtc/dtc \
         KERNEL_DTB="${dtb}" \
         IMAGE=img.axf
}

do_install_append() {
         install -m 0644 ${BW}/img-foundation.axf ${D}/boot/img-foundation.axf
}

do_deploy_append() {
    install -d ${DEPLOYDIR}
    install -m 0644 ${BW}/img.axf ${DEPLOYDIR}/img-${PV}.axf
    install -m 0644 ${BW}/linux-system-foundation.axf ${DEPLOYDIR}/linux-system-foundation-${PV}.axf
    install -m 0644 ${BW}/img-foundation.axf ${DEPLOYDIR}/img-foundation-${PV}.axf
    cd ${DEPLOYDIR}
    ln -sf img-${PV}.axf img.axf
    ln -sf linux-system-foundation-${PV}.axf linux-system-foundation.axf
    ln -sf img-foundation-${PV}.axf img-foundation.axf
}

PACKAGES =+ "kernel-bootwrapper"
FILES_kernel-bootwrapper = "/boot/img-foundation.axf"
