DESCRIPTION = "OpenStack Metering Component"
HOMEPAGE = "https://launchpad.net/ceilometer"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=1dece7821bf3fd70fe1309eaa37d52a2"

SRCNAME = "ceilometer"

SRC_URI = "git://github.com/openstack/${SRCNAME}.git;branch=stable/kilo \
           file://ceilometer.conf \
           file://ceilometer.init \
           file://fix_ceilometer_memory_leak.patch \
"
# dropped for juno:
#   file://ceilometer-builtin-tests-config-location.patch


SRCREV="06b9b908406e078bdbff2aa7e18d19f55ef31037"
PV="2015.1.2+git${SRCPV}"
S = "${WORKDIR}/git"

CEILOMETER_SECRET ?= "12121212"

SERVICECREATE_PACKAGES = "${SRCNAME}-setup ${SRCNAME}-reseller"
KEYSTONE_HOST="${CONTROLLER_IP}"

# USERCREATE_PARAM and SERVICECREATE_PARAM contain the list of parameters to be set.
# If the flag for a parameter in the list is not set here, the default value will be given to that parameter.
# Parameters not in the list will be set to empty.

USERCREATE_PARAM_${SRCNAME}-setup = "name pass tenant role email"
SERVICECREATE_PARAM_${SRCNAME}-setup = "name type description region publicurl adminurl internalurl"
python () {
    flags = {'type':'metering',\
             'description':'OpenStack Metering Service',\
             'publicurl':"'http://${KEYSTONE_HOST}:8777/'",\
             'adminurl':"'http://${KEYSTONE_HOST}:8777/'",\
             'internalurl':"'http://${KEYSTONE_HOST}:8777/'"}

    d.setVarFlags("SERVICECREATE_PARAM_%s-setup" % d.getVar('SRCNAME',True), flags)
}

# Add service user to service tenant as ResellerAdmin role
USERCREATE_PARAM_${SRCNAME}-reseller = "name pass tenant role email"
python () {
    flags = {'role':'ResellerAdmin'}
    d.setVarFlags("USERCREATE_PARAM_%s-reseller" % d.getVar('SRCNAME',True), flags)
}

do_configure_append() {
    # We are using postgresql support, hence this requirement is not valid
    # removing it, to avoid on-target runtime issues
    sed -e "s:MySQL-python::g" -i ${S}/requirements.txt
}

do_install_append() {
    TEMPLATE_CONF_DIR=${S}${sysconfdir}/${SRCNAME}
    CEILOMETER_CONF_DIR=${D}${sysconfdir}/${SRCNAME}

    install -d ${CEILOMETER_CONF_DIR}
    install -m 600 ${WORKDIR}/ceilometer.conf ${CEILOMETER_CONF_DIR}
    install -m 600 ${TEMPLATE_CONF_DIR}/*.json ${CEILOMETER_CONF_DIR}
    install -m 600 ${TEMPLATE_CONF_DIR}/*.yaml ${CEILOMETER_CONF_DIR}

    install -m 600 ${TEMPLATE_CONF_DIR}/api_paste.ini ${CEILOMETER_CONF_DIR}
    if [ -z "${OPENSTACKCHEF_ENABLED}" ]; then
        sed -e "s:%CEILOMETER_SECRET%:${CEILOMETER_SECRET}:g" -i ${CEILOMETER_CONF_DIR}/ceilometer.conf

        sed -e "s:%DB_USER%:${DB_USER}:g" -i ${CEILOMETER_CONF_DIR}/ceilometer.conf
        sed -e "s:%DB_PASSWORD%:${DB_PASSWORD}:g" -i ${CEILOMETER_CONF_DIR}/ceilometer.conf

        sed -e "s:%CONTROLLER_IP%:${CONTROLLER_IP}:g" -i ${CEILOMETER_CONF_DIR}/ceilometer.conf
        sed -e "s:%CONTROLLER_HOST%:${CONTROLLER_HOST}:g" -i ${CEILOMETER_CONF_DIR}/ceilometer.conf

        sed -e "s:%COMPUTE_IP%:${COMPUTE_IP}:g" -i ${CEILOMETER_CONF_DIR}/ceilometer.conf
        sed -e "s:%COMPUTE_HOST%:${COMPUTE_HOST}:g" -i ${CEILOMETER_CONF_DIR}/ceilometer.conf

        sed -e "s:%ADMIN_PASSWORD%:${ADMIN_PASSWORD}:g" -i ${CEILOMETER_CONF_DIR}/ceilometer.conf
        sed -e "s:%SERVICE_TENANT_NAME%:${SERVICE_TENANT_NAME}:g" -i ${CEILOMETER_CONF_DIR}/ceilometer.conf
    fi
    if ${@base_contains('DISTRO_FEATURES', 'sysvinit', 'true', 'false', d)}; then
        install -d ${D}${sysconfdir}/init.d

        sed 's:@suffix@:api:' < ${WORKDIR}/ceilometer.init >${WORKDIR}/ceilometer-api.init.sh
        install -m 0755 ${WORKDIR}/ceilometer-api.init.sh ${D}${sysconfdir}/init.d/ceilometer-api

        sed 's:@suffix@:collector:' < ${WORKDIR}/ceilometer.init >${WORKDIR}/ceilometer-collector.init.sh
        install -m 0755 ${WORKDIR}/ceilometer-collector.init.sh ${D}${sysconfdir}/init.d/ceilometer-collector

        sed 's:@suffix@:agent-central:' < ${WORKDIR}/ceilometer.init >${WORKDIR}/ceilometer-agent-central.init.sh
        install -m 0755 ${WORKDIR}/ceilometer-agent-central.init.sh ${D}${sysconfdir}/init.d/ceilometer-agent-central

        sed 's:@suffix@:agent-compute:' < ${WORKDIR}/ceilometer.init >${WORKDIR}/ceilometer-agent-compute.init.sh
        install -m 0755 ${WORKDIR}/ceilometer-agent-compute.init.sh ${D}${sysconfdir}/init.d/ceilometer-agent-compute

        sed 's:@suffix@:alarm-notifier:' < ${WORKDIR}/ceilometer.init >${WORKDIR}/ceilometer-alarm-notifier.init.sh
        install -m 0755 ${WORKDIR}/ceilometer-alarm-notifier.init.sh ${D}${sysconfdir}/init.d/ceilometer-alarm-notifier

        sed 's:@suffix@:alarm-evaluator:' < ${WORKDIR}/ceilometer.init >${WORKDIR}/ceilometer-alarm-evaluator.init.sh
        install -m 0755 ${WORKDIR}/ceilometer-alarm-evaluator.init.sh ${D}${sysconfdir}/init.d/ceilometer-alarm-evaluator

        sed 's:@suffix@:agent-notification:' < ${WORKDIR}/ceilometer.init >${WORKDIR}/ceilometer-agent-notification.init.sh
        install -m 0755 ${WORKDIR}/ceilometer-agent-notification.init.sh ${D}${sysconfdir}/init.d/ceilometer-agent-notification
    fi

    if [ -e "setup-test-env.sh" ]; then
        cp setup-test-env.sh ${CEILOMETER_CONF_DIR}
    fi 
}

CHEF_SERVICES_CONF_FILES :="\
    ${sysconfdir}/${SRCNAME}/ceilometer.conf \
    "
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
    
    mkdir /var/log/ceilometer
    sudo -u postgres createdb ceilometer
    ceilometer-dbsync
}

inherit setuptools identity hosts update-rc.d default_configs openstackchef monitor

PACKAGES += " ${SRCNAME}-tests"
PACKAGES += "${SRCNAME}-setup ${SRCNAME}-common ${SRCNAME}-api"
PACKAGES += "${SRCNAME}-alarm-notifier ${SRCNAME}-alarm-evaluator"
PACKAGES += "${SRCNAME}-agent-notification"
PACKAGES += "${SRCNAME}-collector ${SRCNAME}-compute ${SRCNAME}-controller"
PACKAGES += " ${SRCNAME}-reseller"

RDEPENDS_${SRCNAME}-tests += " bash"

ALLOW_EMPTY_${SRCNAME}-setup = "1"
ALLOW_EMPTY_${SRCNAME}-reseller = "1"
ALLOW_EMPTY_${SRCNAME}-tests = "1"

FILES_${PN} = "${libdir}/*"

FILES_${SRCNAME}-tests = "${sysconfdir}/${SRCNAME}/setup-test-env.sh"

FILES_${SRCNAME}-common = "${sysconfdir}/${SRCNAME}/* \
"

FILES_${SRCNAME}-api = "${bindir}/ceilometer-api \
                        ${sysconfdir}/init.d/ceilometer-api \
"

FILES_${SRCNAME}-collector = "${bindir}/ceilometer-collector \
                              ${bindir}/ceilometer-collector-udp \
                              ${sysconfdir}/init.d/ceilometer-collector \
"
FILES_${SRCNAME}-alarm-evaluator = "${bindir}/ceilometer-alarm-evaluator \
                                    ${sysconfdir}/init.d/ceilometer-alarm-evaluator \
"

FILES_${SRCNAME}-alarm-notifier = "${bindir}/ceilometer-alarm-notifier \
                                   ${sysconfdir}/init.d/ceilometer-alarm-notifier \
"

FILES_${SRCNAME}-agent-notification = "${bindir}/ceilometer-agent-notification \
                                       ${sysconfdir}/init.d/ceilometer-agent-notification \
"

FILES_${SRCNAME}-compute = "${bindir}/ceilometer-agent-compute \
                            ${sysconfdir}/init.d/ceilometer-agent-compute \
"

FILES_${SRCNAME}-controller = "${bindir}/* \
                               ${localstatedir}/* \
                               ${sysconfdir}/init.d/ceilometer-agent-central \
"

DEPENDS += " \
        python-pip \
        python-pbr \
        "

RDEPENDS_${PN} += " \
	python-ply \
	python-jsonpath-rw \
	python-sqlalchemy \
	python-amqplib \
	python-anyjson \
	python-eventlet \
	python-kombu \
	python-lxml \
	python-routes \
	python-webob \
	python-greenlet \
	python-pastedeploy \
	python-paste \
	python-sqlalchemy-migrate \
	python-stevedore \
	python-suds \
	python-paramiko \
	python-babel \
	python-iso8601 \
	python-setuptools-git \
	python-glanceclient \
	python-keystoneclient \
	python-swiftclient \
	python-ceilometerclient \
	python-oslo.config \
	python-oslo.serialization \
	python-oslo.rootwrap \
	python-tooz \        
	python-msgpack \
	python-pecan \
	python-amqp \
	python-singledispatch \
	python-flask \
	python-werkzeug \
	python-itsdangerous \
	python-happybase \
	python-wsme \
	python-eventlet \
	python-pymongo \
	python-thrift \
	python-simplegeneric \
	python-webtest \
	python-waitress \
	python-pyyaml \
	python-pip \
	python-pytz \
	python-pbr \
	python-croniter \
	python-ipaddr \
	python-pysnmp \
	"

RDEPENDS_${SRCNAME}-controller = "${PN} ${SRCNAME}-common ${SRCNAME}-alarm-notifier ${SRCNAME}-alarm-evaluator ${SRCNAME}-agent-notification ${SRCNAME}-reseller \
                                  postgresql postgresql-client python-psycopg2 tgt"
RDEPENDS_${SRCNAME}-api = "${SRCNAME}-controller"
RDEPENDS_${SRCNAME}-collector = "${SRCNAME}-controller"
RDEPENDS_${SRCNAME}-compute = "${PN} ${SRCNAME}-common python-ceilometerclient libvirt"
RDEPENDS_${SRCNAME}-setup = "postgresql sudo ${SRCNAME}-controller"
RDEPENDS_${SRCNAME}-reseller = "postgresql sudo ${SRCNAME}-controller"

INITSCRIPT_PACKAGES =  "${SRCNAME}-api ${SRCNAME}-collector ${SRCNAME}-compute ${SRCNAME}-controller"
INITSCRIPT_PACKAGES += "${SRCNAME}-alarm-notifier ${SRCNAME}-alarm-evaluator ${SRCNAME}-agent-notification"
INITSCRIPT_NAME_${SRCNAME}-api = "${SRCNAME}-api"
INITSCRIPT_PARAMS_${SRCNAME}-api = "${OS_DEFAULT_INITSCRIPT_PARAMS}"
INITSCRIPT_NAME_${SRCNAME}-collector = "${SRCNAME}-collector"
INITSCRIPT_PARAMS_${SRCNAME}-collector = "${OS_DEFAULT_INITSCRIPT_PARAMS}"
INITSCRIPT_NAME_${SRCNAME}-compute = "${SRCNAME}-agent-compute"
INITSCRIPT_PARAMS_${SRCNAME}-compute = "${OS_DEFAULT_INITSCRIPT_PARAMS}"
INITSCRIPT_NAME_${SRCNAME}-controller = "${SRCNAME}-agent-central"
INITSCRIPT_PARAMS_${SRCNAME}-controller = "${OS_DEFAULT_INITSCRIPT_PARAMS}"
INITSCRIPT_NAME_${SRCNAME}-alarm-notifier = "${SRCNAME}-alarm-notifier"
INITSCRIPT_PARAMS_${SRCNAME}-alarm-notifier = "${OS_DEFAULT_INITSCRIPT_PARAMS}"
INITSCRIPT_NAME_${SRCNAME}-alarm-evaluator = "${SRCNAME}-alarm-evaluator"
INITSCRIPT_PARAMS_${SRCNAME}-alarm-evaluator = "${OS_DEFAULT_INITSCRIPT_PARAMS}"
INITSCRIPT_NAME_${SRCNAME}-agent-notification = "${SRCNAME}-agent-notification"
INITSCRIPT_PARAMS_${SRCNAME}-agent-notification = "${OS_DEFAULT_INITSCRIPT_PARAMS}"

MONITOR_SERVICE_PACKAGES = "${SRCNAME}"
MONITOR_SERVICE_${SRCNAME} = "ceilometer"
