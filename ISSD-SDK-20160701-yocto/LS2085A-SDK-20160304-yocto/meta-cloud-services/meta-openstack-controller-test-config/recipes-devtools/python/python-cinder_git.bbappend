FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI += " file://cinder-test-config.init"

do_install_prepend() {
    # Remove nfs cinder-volume backend driver
    sed -e "s/nfsdriver,//" -i ${WORKDIR}/cinder.conf
}

do_install_append() {
    install -m 0755 ${WORKDIR}/cinder-test-config.init ${D}${sysconfdir}/init.d/cinder-test-config
}

PACKAGES += " ${SRCNAME}-test-config"
FILES_${SRCNAME}-test-config = "${sysconfdir}/init.d/cinder-test-config"

RDEPENDS_${SRCNAME}-tests += " ${SRCNAME}-test-config"

INITSCRIPT_PACKAGES += " ${SRCNAME}-test-config"
INITSCRIPT_NAME_${SRCNAME}-test-config = "cinder-test-config"
INITSCRIPT_PARAMS_${SRCNAME}-test-config = "defaults 95 10"
