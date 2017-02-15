FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://mcs-sshd"
SRC_URI += "file://mcs-sshd_config"

do_install_append() {
    if ${@base_contains('DISTRO_FEATURES', 'OpenLDAP', 'true', 'false', d)}; then
        install -D -m 644 ${WORKDIR}/mcs-sshd ${D}/etc/pam.d/sshd
        install -D -m 644 ${WORKDIR}/mcs-sshd_config ${D}/etc/ssh/sshd_config
    fi
}
