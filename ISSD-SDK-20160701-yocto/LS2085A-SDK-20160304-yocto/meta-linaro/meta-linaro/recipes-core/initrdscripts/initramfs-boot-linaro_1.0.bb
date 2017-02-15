DESCRIPTION = "Simple init script"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://init-boot-linaro.sh"

do_install() {
        install -m 0755 ${WORKDIR}/init-boot-linaro.sh ${D}/init
}

inherit allarch

FILES_${PN} += " /init "
