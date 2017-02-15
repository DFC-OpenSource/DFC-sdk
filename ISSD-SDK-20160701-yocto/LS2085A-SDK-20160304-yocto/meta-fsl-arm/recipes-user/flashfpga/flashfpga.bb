
DESCRIPTION = "Flashfpga"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://README.txt;md5=02afcbdb4c16bb4c656e99cbcedf97ad"
SECTION = "Flashfpga"
PR = "r0"
SRC_URI = "file://flashfpga.tar.bz2;name=tarball"

S="${WORKDIR}"
#export CC="${STAGING_DIR_NATIVE}/usr/bin/aarch64-fsl-linux/aarch64-fsl-linux-gcc"

#EXTRA_OEMAKE += "${CC}"

do_compile() {
     
     echo "${S}"
     cd ${S}/app
     make
     cd ${S}/ver
     make
}

do_install() {
    mkdir -p ${D}/usr/
    mkdir -p ${D}/usr/sbin/
    cp -rf ${S}/app/flash_fpga ${D}/usr/sbin/     
    cp -rf ${S}/ver/fpga_version ${D}/usr/sbin/ 
}
