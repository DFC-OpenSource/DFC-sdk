DESCRIPTION = "ISSD NVMe controller application"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://README.txt;md5=fd78c45fd79cbdb3a08d700ee9b9ff6a"
SECTION = "issd-nvme"
PR = "r0"
SRC_URI = "file://issd-nvme.tar.bz2;name=tarball"
S = "${WORKDIR}"

do_compile() {
    cd ${S}
    make fsluissd
    make fslustdalone
}

do_install() {
    mkdir -p ${D}/usr/
    mkdir -p ${D}/usr/sbin/
    #mkdir -p ${D}/usr/lib/
    #cp -rf ${S}/bdbm.a ${D}/usr/lib/
    cp -rf ${S}/issd_nvme ${D}/usr/sbin/
    cp -rf ${S}/issd_nvme_stda ${D}/usr/sbin/
    echo ${D}
}

PACKAGES =+ "${PN}-dbg ${PN}-dev"
FILES_${PN}-dbg = "${D}/usr/lib/.debug"
FILES_${PN}-dev = "${D}/usr/lib/bdbm.a"
