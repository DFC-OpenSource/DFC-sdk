FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://common-account"
SRC_URI += "file://common-auth"
SRC_URI += "file://common-password"
SRC_URI += "file://common-session"
SRC_URI += "file://common-session-noninteractive"

PACKAGECONFIG ?= "${@base_contains('DISTRO_FEATURES', 'OpenLDAP', 'OpenLDAP', '', d)}"
PACKAGECONFIG[OpenLDAP] = ",,,pam-plugin-mkhomedir nss-pam-ldapd"

do_install_append() {
    if ${@base_contains('DISTRO_FEATURES', 'OpenLDAP', 'true', 'false', d)}; then
        install -m 755 -d ${D}/etc/pam.d/
        install -m 644 ${WORKDIR}/common-account ${D}/etc/pam.d/
        install -m 644 ${WORKDIR}/common-auth ${D}/etc/pam.d/
        install -m 644 ${WORKDIR}/common-password ${D}/etc/pam.d/
        install -m 644 ${WORKDIR}/common-session ${D}/etc/pam.d/
        install -m 644 ${WORKDIR}/common-session-noninteractive ${D}/etc/pam.d/
    fi
}
