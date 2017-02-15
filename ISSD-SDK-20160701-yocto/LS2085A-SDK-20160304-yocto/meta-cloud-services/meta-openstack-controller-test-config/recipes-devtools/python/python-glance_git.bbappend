FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI += " file://glance-test-config.init"

do_install_append() {
    install -m 0755 ${WORKDIR}/glance-test-config.init ${D}${sysconfdir}/init.d/glance-test-config
}

PACKAGES += " ${SRCNAME}-test-config"
FILES_${SRCNAME}-test-config = "${sysconfdir}/init.d/glance-test-config"

RDEPENDS_${SRCNAME}-tests += " ${SRCNAME}-test-config"

INITSCRIPT_PACKAGES += " ${SRCNAME}-test-config"
INITSCRIPT_NAME_${SRCNAME}-test-config = "glance-test-config"
INITSCRIPT_PARAMS_${SRCNAME}-test-config = "defaults 95 10"
