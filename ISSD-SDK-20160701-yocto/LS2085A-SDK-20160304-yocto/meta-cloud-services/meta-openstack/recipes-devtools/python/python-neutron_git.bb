DESCRIPTION = "Neutron (virtual network service)"
HOMEPAGE = "https://launchpad.net/neutron"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=1dece7821bf3fd70fe1309eaa37d52a2"

SRCNAME = "neutron"

SRC_URI = "git://github.com/openstack/${SRCNAME}.git;branch=stable/kilo \
           file://neutron-server.init \
           file://neutron-agent.init \
           file://l3_agent.ini \
           file://dhcp_agent.ini \
           file://metadata_agent.ini \
           file://neutron-dhcp-agent-netns-cleanup.cron \
           file://0001-neutron.conf-jumpstart-nova-state-reporting-configur.patch \
	  "

# TBD: update or drop
# file://uuid_wscheck.patch

SRCREV="b175e03b12527d23c05fc45016e927a20c98d2f1"
PV="2015.1.2+git${SRCPV}"

S = "${WORKDIR}/git"

inherit setuptools update-rc.d identity hosts default_configs openstackchef monitor

SERVICECREATE_PACKAGES = "${SRCNAME}-setup"
KEYSTONE_HOST="${CONTROLLER_IP}"

# USERCREATE_PARAM and SERVICECREATE_PARAM contain the list of parameters to be set.
# If the flag for a parameter in the list is not set here, the default value will be given to that parameter.
# Parameters not in the list will be set to empty.

USERCREATE_PARAM_${SRCNAME}-setup = "name pass tenant role email"
SERVICECREATE_PARAM_${SRCNAME}-setup = "name type description region publicurl adminurl internalurl"
python () {
    flags = {'type':'network',\
             'description':'OpenStack Networking service',\
             'publicurl':"'http://${KEYSTONE_HOST}:9696/'",\
             'adminurl':"'http://${KEYSTONE_HOST}:9696/'",\
             'internalurl':"'http://${KEYSTONE_HOST}:9696/'"}

    d.setVarFlags("SERVICECREATE_PARAM_%s-setup" % d.getVar('SRCNAME',True), flags)
}

do_install_append() {
    TEMPLATE_CONF_DIR=${S}${sysconfdir}/
    NEUTRON_CONF_DIR=${D}${sysconfdir}/neutron

    install -d ${NEUTRON_CONF_DIR}
    install -d ${NEUTRON_CONF_DIR}/plugins/ml2

    install -m 600 ${TEMPLATE_CONF_DIR}/neutron.conf ${NEUTRON_CONF_DIR}/
    install -m 600 ${S}/etc/api-paste.ini ${NEUTRON_CONF_DIR}/
    install -m 600 ${S}/etc/policy.json ${NEUTRON_CONF_DIR}/
    install -m 600 ${TEMPLATE_CONF_DIR}/neutron/plugins/ml2/* ${NEUTRON_CONF_DIR}/plugins/ml2

    # Neutron.conf config changes (replace with .ini file editing)
    sed -e "s:^# core_plugin.*:core_plugin = ml2:g" -i ${NEUTRON_CONF_DIR}/neutron.conf
    sed -e "s:^# service_plugins =.*:service_plugins = router:g" -i ${NEUTRON_CONF_DIR}/neutron.conf
    sed -e "s:^# allow_overlapping_ips = False:allow_overlapping_ips = True:g" -i ${NEUTRON_CONF_DIR}/neutron.conf

    # disable reporting of state changes to nova
    sed -e "s:^# notify_nova_on_port_status_changes.*:notify_nova_on_port_status_changes = False:g" -i ${NEUTRON_CONF_DIR}/neutron.conf
    sed -e "s:^# notify_nova_on_port_data_changes.*:notify_nova_on_port_data_changes = False:g" -i ${NEUTRON_CONF_DIR}/neutron.conf

    sed -e "s:^# connection = sq.*:connection = postgresql\://${ADMIN_USER}\:${ADMIN_PASSWORD}@localhost/neutron:g" -i ${NEUTRON_CONF_DIR}/neutron.conf
    sed -e "s:^#.*rabbit_host=.*:rabbit_host = %CONTROLLER_IP%:" -i ${NEUTRON_CONF_DIR}/neutron.conf

    # ml2_conf.ini changes (replace with .ini file editing)
    sed -e "s:^# type_drivers = .*:type_drivers = gre:g" -i ${NEUTRON_CONF_DIR}/plugins/ml2/ml2_conf.ini
    sed -e "s:^# tenant_network_types = .*:tenant_network_types = gre:g" -i ${NEUTRON_CONF_DIR}/plugins/ml2/ml2_conf.ini
    sed -e "s:^# mechanism_drivers =.*:mechanism_drivers = openvswitch:g" -i ${NEUTRON_CONF_DIR}/plugins/ml2/ml2_conf.ini
    sed -e "s:^# tunnel_id_ranges =.*:tunnel_id_ranges = 1\:1000:g" -i ${NEUTRON_CONF_DIR}/plugins/ml2/ml2_conf.ini

    echo "[ovs]" >> ${NEUTRON_CONF_DIR}/plugins/ml2/ml2_conf.ini
    echo "local_ip = ${MY_IP}" >> ${NEUTRON_CONF_DIR}/plugins/ml2/ml2_conf.ini
    echo "tunnel_type = gre" >> ${NEUTRON_CONF_DIR}/plugins/ml2/ml2_conf.ini
    echo "enable_tunneling = True" >> ${NEUTRON_CONF_DIR}/plugins/ml2/ml2_conf.ini
    echo "[agent]" >> ${NEUTRON_CONF_DIR}/plugins/ml2/ml2_conf.ini
    echo "tunnel_types = gre" >> ${NEUTRON_CONF_DIR}/plugins/ml2/ml2_conf.ini

    PLUGIN=openvswitch
    ARGS="--config-file=${sysconfdir}/${SRCNAME}/neutron.conf --config-file=${sysconfdir}/${SRCNAME}/plugins/ml2/ml2_conf.ini"
    if ${@base_contains('DISTRO_FEATURES', 'sysvinit', 'true', 'false', d)}; then
        install -d ${D}${sysconfdir}/init.d
        sed "s:@plugin@:/etc/neutron/plugins/ml2/ml2_conf.ini:" \
             < ${WORKDIR}/neutron-server.init >${WORKDIR}/neutron-server.init.sh
        install -m 0755 ${WORKDIR}/neutron-server.init.sh ${D}${sysconfdir}/init.d/neutron-server
        sed "s:@suffix@:$PLUGIN:;s:@args@:$ARGS:" < ${WORKDIR}/neutron-agent.init >${WORKDIR}/neutron-$PLUGIN.init.sh
        install -m 0755 ${WORKDIR}/neutron-$PLUGIN.init.sh ${D}${sysconfdir}/init.d/neutron-$PLUGIN-agent
    fi

    AGENT=dhcp
    ARGS="--config-file=${sysconfdir}/${SRCNAME}/neutron.conf --config-file=${sysconfdir}/${SRCNAME}/dhcp_agent.ini"
    if ${@base_contains('DISTRO_FEATURES', 'sysvinit', 'true', 'false', d)}; then
        sed "s:@suffix@:$AGENT:;s:@args@:$ARGS:" < ${WORKDIR}/neutron-agent.init >${WORKDIR}/neutron-$AGENT.init.sh
        install -m 0755 ${WORKDIR}/neutron-$AGENT.init.sh ${D}${sysconfdir}/init.d/neutron-$AGENT-agent
        install -m 600 ${WORKDIR}/${AGENT}_agent.ini ${NEUTRON_CONF_DIR}/
        sed "s:@bindir@:${bindir}:g;s:@confdir@:${sysconfdir}:g" < ${WORKDIR}/neutron-dhcp-agent-netns-cleanup.cron >${WORKDIR}/neutron-dhcp-agent-netns-cleanup
        install -d ${D}${sysconfdir}/cron.d
        install -m 644 ${WORKDIR}/neutron-dhcp-agent-netns-cleanup ${D}${sysconfdir}/cron.d/
    fi

    AGENT=l3
    ARGS="--config-file=${sysconfdir}/${SRCNAME}/neutron.conf --config-file=${sysconfdir}/${SRCNAME}/l3_agent.ini"
    if ${@base_contains('DISTRO_FEATURES', 'sysvinit', 'true', 'false', d)}; then
        sed "s:@suffix@:$AGENT:;s:@args@:$ARGS:" < ${WORKDIR}/neutron-agent.init >${WORKDIR}/neutron-$AGENT.init.sh
        install -m 0755 ${WORKDIR}/neutron-$AGENT.init.sh ${D}${sysconfdir}/init.d/neutron-$AGENT-agent
        install -m 600 ${WORKDIR}/${AGENT}_agent.ini ${NEUTRON_CONF_DIR}/
    fi

    AGENT=metadata
    ARGS="--config-file=${sysconfdir}/${SRCNAME}/neutron.conf --config-file=${sysconfdir}/${SRCNAME}/metadata_agent.ini"
    if ${@base_contains('DISTRO_FEATURES', 'sysvinit', 'true', 'false', d)}; then
        sed "s:@suffix@:$AGENT:;s:@args@:$ARGS:" < ${WORKDIR}/neutron-agent.init >${WORKDIR}/neutron-$AGENT.init.sh
        install -m 0755 ${WORKDIR}/neutron-$AGENT.init.sh ${D}${sysconfdir}/init.d/neutron-$AGENT-agent
        install -m 600 ${WORKDIR}/${AGENT}_agent.ini ${NEUTRON_CONF_DIR}/
    fi
    if [ -z "${OPENSTACKCHEF_ENABLED}" ]; then
        for file in plugins/ml2/ml2_conf.ini neutron.conf metadata_agent.ini; do
        sed -e "s:%SERVICE_TENANT_NAME%:${SERVICE_TENANT_NAME}:g" -i ${NEUTRON_CONF_DIR}/$file
        sed -e "s:%SERVICE_USER%:${SRCNAME}:g" -i ${NEUTRON_CONF_DIR}/$file
        sed -e "s:%SERVICE_PASSWORD%:${SERVICE_PASSWORD}:g" -i ${NEUTRON_CONF_DIR}/$file
        sed -e "s:%METADATA_SHARED_SECRET%:${METADATA_SHARED_SECRET}:g" -i ${NEUTRON_CONF_DIR}/$file
        sed -e "s:%DB_USER%:${DB_USER}:g" -i ${NEUTRON_CONF_DIR}/$file
        sed -e "s:%DB_PASSWORD%:${DB_PASSWORD}:g" -i ${NEUTRON_CONF_DIR}/$file
        sed -e "s:%CONTROLLER_IP%:${CONTROLLER_IP}:g" -i ${NEUTRON_CONF_DIR}/$file
        sed -e "s:%CONTROLLER_HOST%:${CONTROLLER_HOST}:g" -i ${NEUTRON_CONF_DIR}/$file
        done
    fi
    sed -e "s:^auth_host.*:#auth_host:g" -i ${NEUTRON_CONF_DIR}/neutron.conf
    sed -e "s:^auth_port.*:#auth_port:g" -i ${NEUTRON_CONF_DIR}/neutron.conf
    sed -e "s:^auth_protocol.*:#auth_protocol:g" -i ${NEUTRON_CONF_DIR}/neutron.conf
    sed -i '/\[keystone_authtoken\]/aidentity_uri=http://127.0.0.1:8081/keystone/admin/' ${NEUTRON_CONF_DIR}/neutron.conf

    cp run_tests.sh ${NEUTRON_CONF_DIR}
}

pkg_postinst_${SRCNAME}-setup () {
    if [ "x$D" != "x" ]; then
        exit 1
    fi

    # This is to make sure postgres is configured and running
    if ! pidof postmaster > /dev/null; then
        /etc/init.d/postgresql-init
        /etc/init.d/postgresql start
        sleep 2
    fi

    sudo -u postgres createdb neutron
    sudo neutron-db-manage --config-file /etc/neutron/neutron.conf  \
                           --config-file /etc/neutron/plugins/ml2/ml2_conf.ini upgrade head
}

CHEF_SERVICES_CONF_FILES := " \
    ${sysconfdir}/${SRCNAME}/neutron.conf \
    ${sysconfdir}/${SRCNAME}/metadata_agent.ini \
    ${sysconfdir}/${SRCNAME}/plugins/ml2/ml2_conf.ini \
    "
deploychef_services_special_func(){
    #This function is a callback function for the deploychef .bbclass
    #We define this special callback funtion because we are doing 
    #more than a placeholder substitution. The variable CHEF_SERVICES_FILE_NAME
    #is defined in deploychef_framework.bbclass
    if [ -n "${CHEF_SERVICES_FILE_NAME}" ]; then
        sed "s:^# rabbit_host =.*:rabbit_host = %CONTROLLER_IP%:" -i \
        ${CHEF_SERVICES_FILE_NAME}
    fi
}

CHEF_SERVICES_SPECIAL_FUNC := "deploychef_services_special_func"

pkg_postinst_${SRCNAME}-plugin-openvswitch-setup () {
    if [ "x$D" != "x" ]; then
        exit 1
    fi
   
    /etc/init.d/openvswitch-switch start
    ovs-vsctl --no-wait -- --may-exist add-br br-int
}

ALLOW_EMPTY_${SRCNAME}-setup = "1"
ALLOW_EMPTY_${SRCNAME}-plugin-openvswitch-setup = "1"

PACKAGES += " \
     ${SRCNAME}-tests \
     ${SRCNAME} \
     ${SRCNAME}-doc \
     ${SRCNAME}-server \
     ${SRCNAME}-plugin-openvswitch \
     ${SRCNAME}-plugin-ml2 \
     ${SRCNAME}-ml2 \
     ${SRCNAME}-dhcp-agent \
     ${SRCNAME}-l3-agent \
     ${SRCNAME}-metadata-agent \
     ${SRCNAME}-extra-agents \
     ${SRCNAME}-setup \
     ${SRCNAME}-plugin-openvswitch-setup \
     "

FILES_${PN} = "${libdir}/*"

FILES_${SRCNAME}-tests = "${sysconfdir}/${SRCNAME}/run_tests.sh"
RDEPENDS_${SRCNAME}-tests += " bash"


FILES_${SRCNAME} = " \
    ${bindir}/neutron-db-manage \
    ${bindir}/neutron-rootwrap \
    ${bindir}/neutron-debug \
    ${bindir}/neutron-netns-cleanup \
    ${bindir}/neutron-ovs-cleanup \
    ${sysconfdir}/${SRCNAME}/policy.json \
    ${sysconfdir}/${SRCNAME}/neutron.conf \
    ${sysconfdir}/${SRCNAME}/api-paste.ini \
    ${localstatedir}/* \    
    "

FILES_${SRCNAME}-server = "${bindir}/neutron-server \
    ${sysconfdir}/init.d/neutron-server \
    "

FILES_${SRCNAME}-plugin-ml2 = " \
    ${sysconfdir}/${SRCNAME}/plugins/ml2/* \
    "

FILES_${SRCNAME}-plugin-openvswitch = " \
    ${bindir}/neutron-openvswitch-agent \
    ${sysconfdir}/init.d/neutron-openvswitch-agent \
    "

FILES_${SRCNAME}-dhcp-agent = "${bindir}/neutron-dhcp-agent \
    ${bindir}/neutron-dhcp-agent-dnsmasq-lease-update \
    ${sysconfdir}/${SRCNAME}/dhcp_agent.ini \
    ${sysconfdir}/init.d/neutron-dhcp-agent \
    ${sysconfdir}/cron.d/neutron-dhcp-agent-netns-cleanup \
    "

FILES_${SRCNAME}-l3-agent = "${bindir}/neutron-l3-agent \
    ${sysconfdir}/${SRCNAME}/l3_agent.ini \
    ${sysconfdir}/init.d/neutron-l3-agent \
    "

FILES_${SRCNAME}-metadata-agent = "${bindir}/neutron-metadata-agent \
    ${bindir}/neutron-ns-metadata-proxy \
    ${sysconfdir}/${SRCNAME}/metadata_agent.ini \
    ${sysconfdir}/init.d/neutron-metadata-agent \
    "

FILES_${SRCNAME}-extra-agents = "${bindir}/*"

FILES_${SRCNAME}-doc = "${datadir}/*"

DEPENDS += " \
        python-pip \
        python-pbr \
        "

RDEPENDS_${PN} += "python-paste \
	python-pastedeploy \
	python-routes \
	python-amqplib \
	python-anyjson \
	python-alembic \
	python-eventlet \
	python-greenlet \
	python-httplib2 \
	python-iso8601 \
	python-kombu \
	python-netaddr \
	python-neutronclient \
	python-sqlalchemy \
	python-webob \
	python-keystoneclient \
	python-oslo.config \
	python-oslo.rootwrap \
	python-pyudev \
	python-novaclient \
	python-mako \
	python-markupsafe \
	python-pyparsing \
	python-pbr \
	python-jsonrpclib \
	"

RDEPENDS_${SRCNAME} = "${PN} \
        postgresql postgresql-client python-psycopg2"

RDEPENDS_${SRCNAME}-server = "${SRCNAME}"
RDEPENDS_${SRCNAME}-plugin-openvswitch = "${SRCNAME} ${SRCNAME}-plugin-ml2 ${SRCNAME}-plugin-openvswitch-setup openvswitch-switch iproute2 bridge-utils"
RDEPENDS_${SRCNAME}-plugin-openvswitch-setup = "openvswitch-switch "
RDEPENDS_${SRCNAME}-dhcp-agent = "${SRCNAME} dnsmasq dhcp-server dhcp-server-config"
RDEPENDS_${SRCNAME}-l3-agent = "${SRCNAME} ${SRCNAME}-metadata-agent iputils"
RDEPENDS_${SRCNAME}-setup = "postgresql sudo"

RRECOMMENDS_${SRCNAME}-server = "${SRCNAME}-plugin-openvswitch"

INITSCRIPT_PACKAGES = "${SRCNAME}-server ${SRCNAME}-plugin-openvswitch ${SRCNAME}-dhcp-agent ${SRCNAME}-l3-agent ${SRCNAME}-metadata-agent"
INITSCRIPT_NAME_${SRCNAME}-server = "neutron-server"
INITSCRIPT_PARAMS_${SRCNAME}-server = "${OS_DEFAULT_INITSCRIPT_PARAMS}"
INITSCRIPT_NAME_${SRCNAME}-plugin-openvswitch = "neutron-openvswitch-agent"
INITSCRIPT_PARAMS_${SRCNAME}-plugin-openvswitch = "${OS_DEFAULT_INITSCRIPT_PARAMS}"
INITSCRIPT_NAME_${SRCNAME}-dhcp-agent = "neutron-dhcp-agent"
INITSCRIPT_PARAMS_${SRCNAME}-dhcp-agent = "${OS_DEFAULT_INITSCRIPT_PARAMS}"
INITSCRIPT_NAME_${SRCNAME}-l3-agent = "neutron-l3-agent"
INITSCRIPT_PARAMS_${SRCNAME}-l3-agent = "${OS_DEFAULT_INITSCRIPT_PARAMS}"
INITSCRIPT_NAME_${SRCNAME}-metadata-agent = "neutron-metadata-agent"
INITSCRIPT_PARAMS_${SRCNAME}-metadata-agent = "${OS_DEFAULT_INITSCRIPT_PARAMS}"

MONITOR_SERVICE_PACKAGES = "${SRCNAME}"
MONITOR_SERVICE_${SRCNAME} = "neutron"
