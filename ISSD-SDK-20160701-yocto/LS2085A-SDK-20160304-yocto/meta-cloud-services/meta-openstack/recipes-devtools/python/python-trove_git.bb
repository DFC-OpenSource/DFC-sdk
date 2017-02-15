DESCRIPTION = "Trove is Database as a Service for Open Stack."
HOMEPAGE = "https://wiki.openstack.org/wiki/Trove"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=1dece7821bf3fd70fe1309eaa37d52a2"

PR = "r0"
SRCNAME = "trove"

SRC_URI = "git://github.com/openstack/trove.git;branch=stable/kilo \
          file://trove-init \
          "

SRCREV="299d2de4fa1f46be8a023207568dbf81cd67b530"
PV="2015.1.2+git${SRCPV}"
S = "${WORKDIR}/git"

inherit update-rc.d setuptools identity hosts useradd default_configs

SERVICECREATE_PACKAGES = "${SRCNAME}-setup"
KEYSTONE_HOST="${CONTROLLER_IP}"

# USERCREATE_PARAM and SERVICECREATE_PARAM contain the list of parameters to be
# set.  If the flag for a parameter in the list is not set here, the default
# value will be given to that parameter. Parameters not in the list will be set
# to empty.

USERCREATE_PARAM_${SRCNAME}-setup = "name pass tenant role email"
SERVICECREATE_PARAM_${SRCNAME}-setup = "name type description region publicurl adminurl internalurl"
python () {
    flags = {'type':'database',\
             'description':'Trove Database As A Service',\
             'publicurl':"'http://${KEYSTONE_HOST}:8779/v1.0/\$(tenant_id)s'",\
             'adminurl':"'http://${KEYSTONE_HOST}:8779/v1.0/\$(tenant_id)s'",\
             'internalurl':"'http://${KEYSTONE_HOST}:8779/v1.0/\$(tenant_id)s'"}
    d.setVarFlags("SERVICECREATE_PARAM_%s-setup" % d.getVar('SRCNAME',True), flags)
}
SERVICECREATE_PACKAGES[vardeps] += "KEYSTONE_HOST"

do_install_append() {
    TEMPLATE_CONF_DIR=${S}${sysconfdir}/${SRCNAME}
    TROVE_CONF_DIR=${D}${sysconfdir}/${SRCNAME}
    TROVE_LOG_DIR="/var/log/${SRCNAME}"

    ADMIN_USER="admin"
    TROVE_USER="trove"
    TROVE_TENANT="service"
    set -x

    install -d ${D}${localstatedir}/${TROVE_LOG_DIR}
    install -d ${TROVE_CONF_DIR}

    # init.
    if ${@base_contains('DISTRO_FEATURES', 'sysvinit', 'true', 'false', d)};
    then
        install -d ${D}${sysconfdir}/init.d
        for suffix in api taskmanager conductor; do
            SUFFIX_FILE=${D}${sysconfdir}/init.d/trove-${suffix}
            install -m 0755 ${WORKDIR}/trove-init ${SUFFIX_FILE}
            sed -e "s:@suffix@:${suffix}:g" -i ${SUFFIX_FILE}
        done
    fi


    install -d ${D}${localstatedir}/lib/trove

    cp -r ${TEMPLATE_CONF_DIR}/* ${TROVE_CONF_DIR}

    for file in trove.conf trove-conductor.conf trove-taskmanager.conf trove-guestagent.conf; do
        LOG_FILE=`basename $file .conf`

        # Install config files.
        install -m 600 "${TEMPLATE_CONF_DIR}/${file}.sample" \
            "${TROVE_CONF_DIR}/$file"

        # Modify the common parts of the files.
        sed -e "s:#log_dir.*:log_dir = ${TROVE_LOG_DIR}:g" \
                -i ${TROVE_CONF_DIR}/$file

        sed -e "s:^sql_connection = mysql\(.*\):#sql_connection = mysql\1:g" \
                -i ${TROVE_CONF_DIR}/$file

        sed -e "s,^#sql_connection = postgresql://.*,sql_connection = postgresql://${ADMIN_USER}:${ADMIN_PASSWORD}@localhost/trove,g" \
                -i ${TROVE_CONF_DIR}/$file

        sed -i "/sql_connection = postgres.*n/adefault_datastore = postgresql" \
                ${TROVE_CONF_DIR}/$file

        sed -e "s,dns_auth_url = .*,dns_auth_url = http://127.0.0.1:8081/keystone/main/v2.0,g" \
                -i ${TROVE_CONF_DIR}/$file

        sed -e "s:dns_username = .*:dns_username = admin:g" \
                -i ${TROVE_CONF_DIR}/$file

        sed -e "s:#rabbit_host=.*:rabbit_host=${CONTROLLER_IP}:g" \
                -i ${TROVE_CONF_DIR}/$file

        sed -e "s:rabbit_password\(.*\):#rabbit_password\1:g" \
                -i ${TROVE_CONF_DIR}/$file

        sed -e "s,trove_auth_url.*,trove_auth_url = http://${CONTROLLER_IP}:8081/keystone/main/v2.0,g" \
                -i ${TROVE_CONF_DIR}/$file

        sed -e "s:nova_proxy_admin_user.*:nova_proxy_admin_user = ${ADMIN_USER}:g" \
                -i ${TROVE_CONF_DIR}/$file

        sed -e "s:nova_proxy_admin_pass.*:nova_proxy_admin_user = ${ADMIN_PASSWORD}:g" \
                -i ${TROVE_CONF_DIR}/$file

        sed -e "s:nova_proxy_admin_tenant_name.*:nova_proxy_tenant_name = ${TROVE_TENANT}:g" \
                -i ${TROVE_CONF_DIR}/$file

        sed -e "s:#log_file.*:log_file = ${LOG_FILE}.log:g" \
                -i ${TROVE_CONF_DIR}/$file
        for uncomment in rabbit_port nova_compute_url; do
            sed -e "s,#${uncomment}\(.*\),${uncomment}\1,g" \
                    -i ${TROVE_CONF_DIR}/$file
        done


    done


    # Modify api-paste.ini
    sed -e "s:%SERVICE_TENANT_NAME%:${TROVE_TENANT}:g" \
            -i ${TROVE_CONF_DIR}/api-paste.ini

    sed -e "s:%SERVICE_USER%:${TROVE_USER}:g" \
            -i ${TROVE_CONF_DIR}/api-paste.ini

    sed -e "s:%SERVICE_PASSWORD%:${ADMIN_PASSWORD}:g" \
            -i ${TROVE_CONF_DIR}/api-paste.ini

    sed -i "/paste.filter_factory = keystonemiddleware.auth_token/aidentity_uri=http://${CONTROLLER_IP}:8081/keystone/admin/" ${TROVE_CONF_DIR}/api-paste.ini

    # revert location of keystone middleware class.  This will most likely need 
    # to be removed for Juno.
    sed -e "s,paste.filter_factory = keystonemiddleware.auth_token:filter_factory,paste.filter_factory = keystoneclient.middleware.auth_token:filter_factory,g" -i ${TROVE_CONF_DIR}/api-paste.ini

    for delete in auth_host auth_port auth_protocol; do
        sed "/^${delete}\(.*\)/d" -i ${TROVE_CONF_DIR}/api-paste.ini
    done


}


pkg_postinst_${SRCNAME}-setup () {
    # python-trove-setup postinst start
    if [ "x$D" != "x" ]; then
        exit 1
    fi
    source /etc/nova/openrc

    # This is to make sure postgres is configured and running
    if ! pidof postmaster > /dev/null; then
       /etc/init.d/postgresql-init
       /etc/init.d/postgresql start
       sleep 5
    fi

    mkdir /var/log/trove
    # Create database for trove.
    sudo -u postgres createdb trove

    # Create default trove database.
    trove-manage db_sync
    # Create new datastore.
    trove-manage datastore_update "postgresql" ""
    # Set up new version
    trove-manage datastore_version_update "postgresql" "9.1" "postgresql" 1 "postgresql-server-9.1" 1
    # Set new default version.
    trove-manage datastore_update "postgresql" "9.1"
}


USERADD_PACKAGES = "${PN}"
GROUPADD_PARAM_${PN} = "--system trove"
USERADD_PARAM_${PN}  = "--system --home /var/lib/trove -g trove \
                        --no-create-home --shell /bin/false trove"

PROVIDES += " \
    ${SRCNAME} \
    ${SRCNAME}-tests \
    "

PACKAGES += " \
    ${SRCNAME} \
    ${SRCNAME}-api \
    ${SRCNAME}-bin \
    ${SRCNAME}-conductor \
    ${SRCNAME}-setup \
    ${SRCNAME}-taskmanager \
    "

PACKAGES_prepend = " \
    ${SRCNAME}-tests \
    "

FILES_${PN} = " \
    ${libdir}/* \
    "

FILES_${SRCNAME}-tests = " \
    ${libdir}/python*/site-packages/${SRCNAME}/tests/* \
    "

ALLOW_EMPTY_${SRCNAME} = "1"
FILES_${SRCNAME} = " \
    "

FILES_${SRCNAME}-api = " \
    ${sysconfdir}/init.d/trove-api \
    "

FILES_${SRCNAME}-bin = " \
    ${bindir}/* \
    "

FILES_${SRCNAME}-conductor = " \
    ${sysconfdir}/init.d/trove-conductor \
    "

FILES_${SRCNAME}-taskmanager = " \
    ${sysconfdir}/init.d/trove-taskmanager \
    "

FILES_${SRCNAME}-setup = " \
    ${localstatedir}/* \
    ${sysconfdir}/${SRCNAME}/* \
    "



DEPENDS += " \
    python-pbr \
    python-pip \
    "

RDEPENDS_${PN} += " \
    python-babel \
    python-cinderclient \
    python-eventlet \
    python-falcon \
    python-glanceclient \
    python-heatclient \
    python-httplib2 \
    python-iso8601 \
    python-jinja2 \
    python-jsonschema \
    python-keystoneclient \
    python-kombu \
    python-lxml \
    python-netaddr \
    python-neutronclient \
    python-novaclient \
    python-oslo.config \
    python-passlib \
    python-paste \
    python-pastedeploy \
    python-routes \
    python-sqlalchemy-migrate \
    python-swiftclient \
    python-webob \
    uwsgi \
    "

RDEPENDS_${SRCNAME} = " \
    ${PN} \
    ${SRCNAME}-api \
    ${SRCNAME}-bin \
    ${SRCNAME}-conductor \
    ${SRCNAME}-setup \
    ${SRCNAME}-taskmanager \
    troveclient \
    "

RDEPENDS_${SRCNAME}-api = " \
    ${SRCNAME}-setup \
    "

RDEPENDS_${SRCNAME}-bin = " \
    ${PN} \
    "

RDEPENDS_${SRCNAME}-conductor = " \
    ${SRCNAME}-setup \
    "

RDEPENDS_${SRCNAME}-setup = " \
    ${PN} \
    ${SRCNAME}-bin \
    keystone-setup \
    postgresql \
    postgresql-client \
    python-keystoneclient \
    python-novaclient \
    sudo \
    "

RDEPENDS_${SRCNAME}-taskmanager = " \
    ${SRCNAME}-setup \
    "

RDEPENDS_${SRCNAME}-tests += " \
    python-mock \
    python-pexpect \
    "


INITSCRIPT_PACKAGES = "${SRCNAME}-api ${SRCNAME}-conductor ${SRCNAME}-taskmanager"

INITSCRIPT_NAME_${SRCNAME}-api = "trove-api"
INITSCRIPT_PARAMS_${SRCNAME}-api = "${OS_DEFAULT_INITSCRIPT_PARAMS}"

INITSCRIPT_NAME_${SRCNAME}-conductor = "trove-conductor"
INITSCRIPT_PARAMS_${SRCNAME}-conductor = "${OS_DEFAULT_INITSCRIPT_PARAMS}"

INITSCRIPT_NAME_${SRCNAME}-taskmanager = "trove-taskmanager"
INITSCRIPT_PARAMS_${SRCNAME}-taskmanager = "${OS_DEFAULT_INITSCRIPT_PARAMS}"
