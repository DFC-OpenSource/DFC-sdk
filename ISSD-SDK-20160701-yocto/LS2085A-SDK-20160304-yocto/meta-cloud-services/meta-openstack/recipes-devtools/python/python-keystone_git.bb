DESCRIPTION = "Authentication service for OpenStack"
HOMEPAGE = "http://www.openstack.org"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=1dece7821bf3fd70fe1309eaa37d52a2"

SRCNAME = "keystone"

SRC_URI = "git://github.com/openstack/${SRCNAME}.git;branch=stable/kilo \
           file://keystone.conf \
           file://identity.sh \
           file://keystone \
           file://convert_keystone_backend.py \
           file://wsgi-keystone.conf \
           "

# TBD: update or drop
# file://keystone-search-in-etc-directory-for-config-files.patch 
# file://keystone-remove-git-commands-in-tests.patch 
# file://keystone-explicitly-import-localcontext-from-oslo.me.patch

SRCREV="8a2da01cb8b4a8369af4f5cf5cc7e20b1bf4453a"
PV="2015.1.2+git${SRCPV}"

S = "${WORKDIR}/git"

inherit setuptools update-rc.d identity hosts default_configs openstackchef monitor

SERVICE_TOKEN = "password"
TOKEN_FORMAT ?= "PKI"

LDAP_DN ?= "dc=my-domain,dc=com"

SERVICECREATE_PACKAGES = "${SRCNAME}-setup"
KEYSTONE_HOST="${CONTROLLER_IP}"

# USERCREATE_PARAM and SERVICECREATE_PARAM contain the list of parameters to be
# set.  If the flag for a parameter in the list is not set here, the default
# value will be given to that parameter. Parameters not in the list will be set
# to empty.

USERCREATE_PARAM_${SRCNAME}-setup = "name pass tenant role email"
python () {
    flags = {'name':'${ADMIN_USER}',\
             'pass':'${ADMIN_PASSWORD}',\
             'tenant':'${ADMIN_TENANT}',\
             'role':'${ADMIN_ROLE}',\
             'email':'${ADMIN_USER_EMAIL}',\
            }
    d.setVarFlags("USERCREATE_PARAM_%s-setup" % d.getVar('SRCNAME',True), flags)
}

SERVICECREATE_PARAM_${SRCNAME}-setup = "name type description region publicurl adminurl internalurl"
python () {
    flags = {'type':'identity',\
             'description':'OpenStack Identity',\
             'publicurl':"'http://${KEYSTONE_HOST}:8081/keystone/main/v2.0'",\
             'adminurl':"'http://${KEYSTONE_HOST}:8081/keystone/admin/v2.0'",\
             'internalurl':"'http://${KEYSTONE_HOST}:8081/keystone/main/v2.0'"}
    d.setVarFlags("SERVICECREATE_PARAM_%s-setup" % d.getVar('SRCNAME',True), flags)
}

do_install_append() {

    KEYSTONE_CONF_DIR=${D}${sysconfdir}/keystone
    KEYSTONE_PACKAGE_DIR=${D}${PYTHON_SITEPACKAGES_DIR}/keystone

    APACHE_CONF_DIR=${D}${sysconfdir}/apache2/conf.d/
    KEYSTONE_PY_DIR=${D}${datadir}/openstack-dashboard/openstack_dashboard/api/
    KEYSTONE_CGI_DIR=${D}${localstatedir}/www/cgi-bin/keystone/

    # Apache needs to read the configs.
    install -m 755 -d ${KEYSTONE_CONF_DIR}
    install -m 755 -d ${APACHE_CONF_DIR}

    install -d ${D}${localstatedir}/log/${SRCNAME}
    install -g users -m 755 -d ${KEYSTONE_CGI_DIR}
    install -g users -m 755 -d ${KEYSTONE_PY_DIR}

    # Apache needs to read the keystone.conf
    install -m 644 ${WORKDIR}/keystone.conf ${KEYSTONE_CONF_DIR}/
    # Apache needs to read the wsgi-keystone.conf
    install -m 644 ${WORKDIR}/wsgi-keystone.conf ${APACHE_CONF_DIR}
    install -m 755 ${WORKDIR}/identity.sh ${KEYSTONE_CONF_DIR}/
    install -m 600 ${S}${sysconfdir}/logging.conf.sample \
        ${KEYSTONE_CONF_DIR}/logging.conf
    install -m 600 ${S}${sysconfdir}/keystone.conf.sample \
        ${KEYSTONE_CONF_DIR}/keystone.conf.sample
    # Apache user needs to read these files.
    install -m 644 ${S}${sysconfdir}/policy.json \
        ${KEYSTONE_CONF_DIR}/policy.json
    install -m 644 ${S}${sysconfdir}/keystone-paste.ini \
        ${KEYSTONE_CONF_DIR}/keystone-paste.ini
    install -g users -m 644 ${S}/httpd/keystone.py \
        ${KEYSTONE_PY_DIR}/keystone-httpd.py
    install -g users -m 644 ${S}/httpd/keystone.py \
        ${KEYSTONE_CGI_DIR}/admin
    install -g users -m 644 ${S}/httpd/keystone.py \
        ${KEYSTONE_CGI_DIR}/main

    cp -r ${S}/examples ${KEYSTONE_PACKAGE_DIR}

    if ${@base_contains('DISTRO_FEATURES', 'sysvinit', 'true', 'false', d)};
    then
        install -d ${D}${sysconfdir}/init.d
        install -m 0755 ${WORKDIR}/keystone ${D}${sysconfdir}/init.d/keystone
    fi

    sed "/# admin_endpoint = .*/a \
        public_endpoint = http://%CONTROLLER_IP%:8081/keystone/main/ " \
        -i ${KEYSTONE_CONF_DIR}/keystone.conf

    sed "/# admin_endpoint = .*/a \
        admin_endpoint = http://%CONTROLLER_IP%:8081/keystone/admin/ " \
        -i ${KEYSTONE_CONF_DIR}/keystone.conf
    
    if [ -z "${OPENSTACKCHEF_ENABLED}" ]; then
        sed -e "s:%SERVICE_TOKEN%:${SERVICE_TOKEN}:g" \
            -i ${KEYSTONE_CONF_DIR}/keystone.conf
        sed -e "s:%DB_USER%:${DB_USER}:g" -i ${KEYSTONE_CONF_DIR}/keystone.conf
        sed -e "s:%DB_PASSWORD%:${DB_PASSWORD}:g" \
            -i ${KEYSTONE_CONF_DIR}/keystone.conf

        sed -e "s:%CONTROLLER_IP%:${CONTROLLER_IP}:g" \
            -i ${KEYSTONE_CONF_DIR}/keystone.conf
        sed -e "s:%CONTROLLER_IP%:${CONTROLLER_IP}:g" \
            -i ${KEYSTONE_CONF_DIR}/identity.sh

        sed -e "s:%TOKEN_FORMAT%:${TOKEN_FORMAT}:g" \
            -i ${KEYSTONE_CONF_DIR}/keystone.conf
        
        sed -e "s/%ADMIN_PASSWORD%/${ADMIN_PASSWORD}/g" \
            -i ${D}${sysconfdir}/init.d/keystone
        sed -e "s/%SERVICE_PASSWORD%/${SERVICE_PASSWORD}/g" \
            -i ${D}${sysconfdir}/init.d/keystone
        sed -e "s/%SERVICE_TENANT_NAME%/${SERVICE_TENANT_NAME}/g" \
            -i ${D}${sysconfdir}/init.d/keystone
    fi    
    
    install -d ${KEYSTONE_PACKAGE_DIR}/tests/tmp

    if [ -e "${KEYSTONE_PACKAGE_DIR}/tests/test_overrides.conf" ];then
        sed -e "s:%KEYSTONE_PACKAGE_DIR%:${PYTHON_SITEPACKAGES_DIR}/keystone:g" \
            -i ${KEYSTONE_PACKAGE_DIR}/tests/test_overrides.conf
    fi

    cp run_tests.sh ${KEYSTONE_CONF_DIR}


    if ${@base_contains('DISTRO_FEATURES', 'OpenLDAP', 'true', 'false', d)};
    then
        sed -i -e '/^\[identity\]/a \
driver = keystone.identity.backends.hybrid_identity.Identity \
\
[assignment]\
driver = keystone.assignment.backends.hybrid_assignment.Assignment\
' ${D}${sysconfdir}/keystone/keystone.conf

        sed -i -e '/^\[ldap\]/a \
url = ldap://localhost \
user = cn=Manager,${LDAP_DN} \
password = secret \
suffix = ${LDAP_DN} \
use_dumb_member = True \
\
user_tree_dn = ou=Users,${LDAP_DN} \
user_attribute_ignore = enabled,email,tenants,default_project_id \
user_id_attribute = uid \
user_name_attribute = uid \
user_mail_attribute = email \
user_pass_attribute = keystonePassword \
\
tenant_tree_dn = ou=Groups,${LDAP_DN} \
tenant_desc_attribute = description \
tenant_domain_id_attribute = businessCategory \
tenant_attribute_ignore = enabled \
tenant_objectclass = groupOfNames \
tenant_id_attribute = cn \
tenant_member_attribute = member \
tenant_name_attribute = ou \
\
role_attribute_ignore = enabled \
role_objectclass = groupOfNames \
role_member_attribute = member \
role_id_attribute = cn \
role_name_attribute = ou \
role_tree_dn = ou=Roles,${LDAP_DN} \
' ${D}${sysconfdir}/keystone/keystone.conf

        install -m 0755 ${WORKDIR}/convert_keystone_backend.py \
            ${D}${sysconfdir}/keystone/convert_keystone_backend.py
    fi
}

CHEF_SERVICES_CONF_FILES := "\
    ${sysconfdir}/${SRCNAME}/keystone.conf \
    ${sysconfdir}/${SRCNAME}/identity.sh \
    ${sysconfdir}/init.d/keystone \
    "

pkg_postinst_${SRCNAME}-setup () {
    # python-keystone postinst start
    if [ "x$D" != "x" ]; then
        exit 1
    fi

    # This is to make sure postgres is configured and running
    if ! pidof postmaster > /dev/null; then
        /etc/init.d/postgresql-init
        /etc/init.d/postgresql start
        sleep 2
    fi

    # This is to make sure keystone is configured and running
    PIDFILE="/var/run/keystone-all.pid"
    if [ -z `cat $PIDFILE 2>/dev/null` ]; then
        sudo -u postgres createdb keystone
        keystone-manage db_sync
        keystone-manage pki_setup --keystone-user=root --keystone-group=daemon

        if ${@base_contains('DISTRO_FEATURES', 'OpenLDAP', 'true', 'false', d)}; then
            /etc/init.d/openldap start
        fi
        /etc/init.d/keystone start
    fi
}

# By default tokens are expired after 1 day so by default we can set
# this token flush cronjob to run every 2 days
KEYSTONE_TOKEN_FLUSH_TIME ??= "0 0 */2 * *"

pkg_postinst_${SRCNAME}-cronjobs () {
    # By default keystone expired tokens are not automatic removed out of the
    # database.  So we create a cronjob for cleaning these expired tokens.
    echo "${KEYSTONE_TOKEN_FLUSH_TIME} root /usr/bin/keystone-manage token_flush" >> /etc/crontab
}

PACKAGES += " ${SRCNAME}-tests ${SRCNAME} ${SRCNAME}-setup ${SRCNAME}-cronjobs"

ALLOW_EMPTY_${SRCNAME}-setup = "1"

ALLOW_EMPTY_${SRCNAME}-cronjobs = "1"

FILES_${PN} = "${libdir}/* \
    "

FILES_${SRCNAME}-tests = "${sysconfdir}/${SRCNAME}/run_tests.sh"

FILES_${SRCNAME} = "${bindir}/* \
    ${sysconfdir}/${SRCNAME}/* \
    ${sysconfdir}/init.d/* \
    ${localstatedir}/* \
    ${datadir}/openstack-dashboard/openstack_dashboard/api/keystone-httpd.py \
    ${sysconfdir}/apache2/conf.d/wsgi-keystone.conf \
    "

DEPENDS += " \
        python-pip \
        python-pbr \
        "

RDEPENDS_${PN} += " \
        python-pycadf \
        python-oslo.db \
        python-pam \
        python-webob \
        python-eventlet \
        python-greenlet \
        python-pastedeploy \
        python-paste \
        python-routes \
        python-sqlalchemy \
        python-sqlalchemy-migrate \
        python-passlib \
        python-lxml \
        python-iso8601 \
        python-keystoneclient \
        python-openstack-nose \
        python-oslo.config \
        python-dogpile.core \
        python-dogpile.cache \
        python-pbr \
        python-pysaml2 \
        python-oslo.policy \
        python-oslo.utils \
        python-oauthlib \
        python-fixtures \
        python-oslotest \
        python-cryptography \
        python-functools32 \
        "

RDEPENDS_${SRCNAME}-tests += " bash"

PACKAGECONFIG ?= "${@base_contains('DISTRO_FEATURES', 'OpenLDAP', 'OpenLDAP', '', d)}"
PACKAGECONFIG[OpenLDAP] = ",,,python-ldap python-keystone-hybrid-backend"

# TODO:
#    if DISTRO_FEATURE contains "tempest" then add *-tests to the main RDEPENDS

RDEPENDS_${SRCNAME} = " \
    ${PN} \
    postgresql \
    postgresql-client \
    python-psycopg2 \
    apache2 \
    "

RDEPENDS_${SRCNAME}-setup = "postgresql sudo ${SRCNAME}"
RDEPENDS_${SRCNAME}-cronjobs = "cronie ${SRCNAME}"

INITSCRIPT_PACKAGES = "${SRCNAME}"
INITSCRIPT_NAME_${SRCNAME} = "keystone"
INITSCRIPT_PARAMS_${SRCNAME} = "${OS_DEFAULT_INITSCRIPT_PARAMS}"

MONITOR_SERVICE_PACKAGES = "${SRCNAME}"
MONITOR_SERVICE_${SRCNAME} = "keystone"
