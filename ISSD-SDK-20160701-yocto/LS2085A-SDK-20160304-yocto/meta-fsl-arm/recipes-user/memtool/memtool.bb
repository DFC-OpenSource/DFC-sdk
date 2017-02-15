DESCRIPTION = "Memtool"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://README.txt;md5=d8f731323a8230f591c1929428831f5e"
SECTION = "Memtool"
PR = "r0"
SRC_URI = "file://memtool.tar.bz2;name=tarball"

S="${WORKDIR}"
#export CC="${STAGING_DIR_NATIVE}/usr/bin/aarch64-fsl-linux/aarch64-fsl-linux-gcc"

#EXTRA_OEMAKE += "${CC}"

do_compile() {
     
     echo "${S}"
     echo "${CC}"
     cd ${S}/md
     make
     cd ${S}/mw
     make
}

do_install() {
    mkdir -p ${D}/usr/
    mkdir -p ${D}/usr/sbin/
    cp -rf ${S}/md/md* ${D}/usr/sbin/   
    cp -rf ${S}/mw/mw* ${D}/usr/sbin/   
}
