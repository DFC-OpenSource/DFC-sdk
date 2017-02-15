DESCRIPTION = "i2capp"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://README.txt;md5=af40738f3b45803a0b4ade1686724620"
SECTION = "I2C"
PR = "r0"
SRC_URI = "file://i2capp.c \
	   file://README.txt \
	   "

S="${WORKDIR}"
#export CC="${STAGING_DIR_NATIVE}/usr/bin/aarch64-fsl-linux/aarch64-fsl-linux-gcc"

#EXTRA_OEMAKE += "${CC}"

do_compile() {
     cd ${S}
     ${CC} i2capp.c -o i2capp 
}

do_install() {
    mkdir -p ${D}/usr/
    mkdir -p ${D}/usr/sbin/
    cp -rf ${S}/i2capp ${D}/usr/sbin/   
}
