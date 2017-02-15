DEPEND_${PN} += "cyrus-sasl" 
RDEPEND_${PN} += "libsasl2-modules"

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"
SRC_URI += "file://initscript"
SRC_URI += "file://ops-base.ldif"

LDAP_DN ?= "dc=my-domain,dc=com"
LDAP_DATADIR ?= "/etc/openldap-data/"

OPENLDAP_LIBEXECDIR = "/usr/libexec"

EXTRA_OECONF += "--libexecdir=${OPENLDAP_LIBEXECDIR}"

do_install_append() {
    install -D -m 0755 ${WORKDIR}/initscript ${D}${sysconfdir}/init.d/openldap
    sed -i -e 's/%DEFAULT_DN%/${LDAP_DN}/g' ${D}${sysconfdir}/init.d/openldap
    sed -i -e 's#%LDAP_DATADIR%#${LDAP_DATADIR}#g' ${D}${sysconfdir}/init.d/openldap
    sed -i -e 's#%LIBEXEC%#${OPENLDAP_LIBEXECDIR}#g' ${D}${sysconfdir}/init.d/openldap

    # This is duplicated in /etc/openldap and is for slapd
    rm -f ${D}${localstatedir}/openldap-data/DB_CONFIG.example
    rmdir "${D}${localstatedir}/run"
    rmdir --ignore-fail-on-non-empty "${D}${localstatedir}"

    # remove symlinks for backends, recreating in postinstall
    rm -f ${D}/${OPENLDAP_LIBEXECDIR}/openldap/*.so

    sed -i -e '/^include\s*/a \
include         /etc/openldap/schema/cosine.schema \
include         /etc/openldap/schema/nis.schema \
include         /etc/openldap/schema/inetorgperson.schema \
include         /etc/openldap/schema/misc.schema' \
	${D}/etc/openldap/slapd.conf

    sed -i -e '/^# Load dynamic backend modules:/a \
modulepath      ${OPENLDAP_LIBEXECDIR}/openldap \
moduleload      back_bdb.la' \
	${D}/etc/openldap/slapd.conf

    sed -i -e 's#^pidfile\s*.*$#pidfile ${LDAP_DATADIR}/slapd.pid#' ${D}/etc/openldap/slapd.conf
    sed -i -e 's#^argsfile\s*.*$#argsfile ${LDAP_DATADIR}/slapd.args#' ${D}/etc/openldap/slapd.conf
    sed -i -e 's#^directory\s*.*$#directory ${LDAP_DATADIR}/#' ${D}/etc/openldap/slapd.conf

    sed -i -e 's/dc=my-domain,dc=com/${LDAP_DN}/g' ${D}/etc/openldap/slapd.conf

    # modify access perms for ldap/authentication
    sed -i -e '$a\
\
access to attrs=userPassword \
        by self write \
        by anonymous auth \
        by * none \
\
access to * \
        by self write \
        by * read' \
        ${D}/etc/openldap/slapd.conf

    install -D -m 0644 ${WORKDIR}/ops-base.ldif ${D}/etc/openldap/ops-base.ldif
    sed -i -e 's/dc=my-domain,dc=com/${LDAP_DN}/g' ${D}/etc/openldap/ops-base.ldif

    mkdir ${D}/${LDAP_DATADIR}
}

inherit update-rc.d

INITSCRIPT_NAME = "openldap"
INITSCRIPT_PARAMS = "defaults"

FILES_${PN} += "${OPENLDAP_LIBEXECDIR}/*"
FILES_${PN}-dbg += "${OPENLDAP_LIBEXECDIR}/openldap/.debug ${OPENLDAP_LIBEXECDIR}/.debug"
