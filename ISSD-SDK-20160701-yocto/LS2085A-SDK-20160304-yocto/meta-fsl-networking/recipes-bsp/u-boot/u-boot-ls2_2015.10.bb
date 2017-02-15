require recipes-bsp/u-boot/u-boot.inc
inherit fsl-u-boot-localversion

LICENSE = "GPLv2 & BSD-3-Clause & BSD-2-Clause & LGPL-2.0 & LGPL-2.1"
LIC_FILES_CHKSUM = " \
    file://Licenses/gpl-2.0.txt;md5=b234ee4d69f5fce4486a80fdaf4a4263 \
    file://Licenses/bsd-2-clause.txt;md5=6a31f076f5773aabd8ff86191ad6fdd5 \
    file://Licenses/bsd-3-clause.txt;md5=4a1190eac56a9db675d58ebe86eaf50c \
    file://Licenses/lgpl-2.0.txt;md5=5f30f0716dfdd0d91eb439ebec522ec2 \
    file://Licenses/lgpl-2.1.txt;md5=4fbd65380cdd255951079008b364516c \
"
PV = "2015.10+fslgit"

PROVIDES += "u-boot"

#SRC_URI = "git://sw-stash.freescale.net/scm/sdk/u-boot-devel.git;branch=layerscape;protocol=http"
#SRCREV = "3242b20af55d63c2a0181ecb49d4533adb87ebd3"

SRC_URI = "git://192.168.100.20:8080/fslu_nvme_uboot_ear6.git;protocol=http;nobranch=1"
SRCREV = "7787688df9eb892b11ea9e96e4309da2e727773f"
#SRCREV = "${AUTOREV}"

LOCALVERSION = "${SDK_VERSION}"
UBOOT_BINARY = "u-boot-dtb.${UBOOT_SUFFIX}"

S = "${WORKDIR}/git"

do_compile_append () {
    if [ "x${UBOOT_CONFIG}" != "x" ]; then
        for config in ${UBOOT_MACHINE}; do
            i=`expr $i + 1`;
            UBOOT_SOURCE=${UBOOT_BINARY}
            for type in ${UBOOT_CONFIG}; do
                j=`expr $j + 1`;
                if [ $j -eq $i ]; then
                    case "${type}" in
                        nor|qspi)
                            UBOOT_SOURCE=u-boot-dtb.bin;;
                        nand)
                            UBOOT_SOURCE=u-boot-with-spl.bin;;
                    esac
                    cp ${S}/${config}/${UBOOT_SOURCE} ${S}/${config}/u-boot-${type}.${UBOOT_SUFFIX}
                fi
            done
            unset  j
        done
        unset i
    fi
}

do_install_append (){
    rm -f ${D}/boot/${UBOOT_BINARY}
    if [ "x${UBOOT_CONFIG}" != "x" ]; then
        for config in ${UBOOT_MACHINE}; do
            i=`expr $i + 1`;
            for type in ${UBOOT_CONFIG}; do
                j=`expr $j + 1`;
                if [ $j -eq $i ]; then
                    ln -sf u-boot-${type}-${PV}-${PR}.${UBOOT_SUFFIX} ${D}/boot/u-boot-${type}.${UBOOT_SUFFIX}
                fi
            done
            unset  j
        done
        unset i
    fi
}

do_deploy_append (){
    rm -f ${DEPLOYDIR}/${UBOOT_BINARY}
    if [ "x${UBOOT_CONFIG}" != "x" ]; then
        for config in ${UBOOT_MACHINE}; do
            i=`expr $i + 1`;
            for type in ${UBOOT_CONFIG}; do
                j=`expr $j + 1`;
                if [ $j -eq $i ]; then
                    ln -sf u-boot-${type}-${PV}-${PR}.${UBOOT_SUFFIX} ${DEPLOYDIR}/u-boot-${type}.${UBOOT_SUFFIX}
                fi
            done
            unset  j
        done
        unset i
    fi
}
addtask deploy after do_install
