DESCRIPTION = "Services for discovering, registering and retrieving virtual machine images"
HOMEPAGE = "http://glance.openstack.org/"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=1dece7821bf3fd70fe1309eaa37d52a2"

SRCNAME = "glance"

SRC_URI = "git://github.com/openstack/${SRCNAME}.git;branch=stable/kilo \
           file://glance.init \
           "

# removed. juno uses store library: file://0001-glance-store-only-load-known-stores-not-all-stores.patch
#                                   file://glance-change-builtin-tests-config-location.patch

SRCREV="0303c5c3e43da29501b8b26c82e5a0cc9b7a3ed7"
PV="2015.1.2+git${SRCPV}"

S = "${WORKDIR}/git"

inherit setuptools update-rc.d identity default_configs hosts openstackchef monitor

GLANCE_DEFAULT_STORE ?= "file"
GLANCE_KNOWN_STORES ?= "glance.store.rbd.Store,\
 glance.store.swift.Store,\
 glance.store.cinder.Store,\
 glance.store.filesystem.Store,\
 glance.store.http.Store"


SERVICECREATE_PACKAGES = "${SRCNAME}-setup"
KEYSTONE_HOST="${CONTROLLER_IP}"

# USERCREATE_PARAM and SERVICECREATE_PARAM contain the list of parameters to be set.
# If the flag for a parameter in the list is not set here, the default value will be given to that parameter.
# Parameters not in the list will be set to empty.

USERCREATE_PARAM_${SRCNAME}-setup = "name pass tenant role email"
SERVICECREATE_PARAM_${SRCNAME}-setup = "name type description region publicurl adminurl internalurl"
python () {
    flags = {'type':'image',\
             'description':'OpenStack Image Service',\
             'publicurl':"'http://${KEYSTONE_HOST}:9292/v2'",\
             'adminurl':"'http://${KEYSTONE_HOST}:9292/v2'",\
             'internalurl':"'http://${KEYSTONE_HOST}:9292/v2'"}

    d.setVarFlags("SERVICECREATE_PARAM_%s-setup" % d.getVar('SRCNAME',True), flags)
}
SERVICECREATE_PACKAGES[vardeps] += "KEYSTONE_HOST"

do_install_prepend() {
    sed 's:%PYTHON_SITEPACKAGES_DIR%:${PYTHON_SITEPACKAGES_DIR}:g' -i ${S}/${SRCNAME}/tests/functional/__init__.py
    sed 's:%PYTHON_SITEPACKAGES_DIR%:${PYTHON_SITEPACKAGES_DIR}:g' -i ${S}/${SRCNAME}/tests/unit/base.py
    sed 's:%PYTHON_SITEPACKAGES_DIR%:${PYTHON_SITEPACKAGES_DIR}:g' -i ${S}/${SRCNAME}/tests/utils.py
}

do_install_append() {
    TEMPLATE_CONF_DIR=${S}${sysconfdir}
    GLANCE_CONF_DIR=${D}${sysconfdir}/glance
    
    for file in api registry cache
    do
        install -m 0600 ${TEMPLATE_CONF_DIR}/glance-$file.conf ${WORKDIR}
        sed -e "s!^#connection =.*!connection = postgresql://%DB_USER%:%DB_PASSWORD%@localhost/glance!g" \
        -i ${WORKDIR}/glance-$file.conf
        sed -i '/\[keystone_authtoken\]/aidentity_uri=http://127.0.0.1:8081/keystone/admin' ${WORKDIR}/glance-$file.conf
    done

    sed -e "s:^filesystem_store_datadir =.*:filesystem_store_datadir = ${sysconfdir}/${SRCNAME}/images/:g" \
        -i ${WORKDIR}/glance-api.conf

    # send samples to rabbitmq for ceilometer integration
    sed -e "s:^# notification_driver = noop:notification_driver = rabbit:g" \
        -i ${WORKDIR}/glance-api.conf         

    sed 's:^default_store =.*:default_store = ${GLANCE_DEFAULT_STORE}:g' -i ${WORKDIR}/glance-api.conf
    sed 's:^swift_store_auth_address =.*:swift_store_auth_address = http\://127.0.0.1\:8081/keystone/main/:g' -i ${WORKDIR}/glance-api.conf
    sed 's:^swift_store_user =.*:swift_store_user = %SERVICE_TENANT_NAME%\:${SRCNAME}:g' -i ${WORKDIR}/glance-api.conf
    sed 's:^swift_store_key =.*:swift_store_key = %SERVICE_PASSWORD%:g' -i ${WORKDIR}/glance-api.conf
    sed 's:^swift_store_create_container_on_put =.*:swift_store_create_container_on_put = True:g' -i ${WORKDIR}/glance-api.conf

    # multi line match, replace the known stores with the ones we support.
    sed '1!N; s:#known_stores = glance.store.*\n.*#.*glance.store.http.*:known_stores = ${GLANCE_KNOWN_STORES}:g' -i ${WORKDIR}/glance-api.conf

    install -d ${GLANCE_CONF_DIR}
    install -m 600 ${WORKDIR}/glance-registry.conf ${GLANCE_CONF_DIR}/
    install -m 600 ${WORKDIR}/glance-api.conf ${GLANCE_CONF_DIR}/
    install -m 600 ${WORKDIR}/glance-cache.conf ${GLANCE_CONF_DIR}/

    install -m 600 ${S}/etc/glance-registry-paste.ini ${GLANCE_CONF_DIR}/
    install -m 600 ${S}/etc/glance-api-paste.ini ${GLANCE_CONF_DIR}/
    install -m 600 ${S}/etc/policy.json ${GLANCE_CONF_DIR}/
    install -m 600 ${S}/etc/schema-image.json ${GLANCE_CONF_DIR}/

    install -d ${GLANCE_CONF_DIR}/images
    install -d ${D}${localstatedir}/lib/glance/image_cache

    install -d ${D}${localstatedir}/log/${SRCNAME}

    if [ -z "${OPENSTACKCHEF_ENABLED}" ]; then
        for file in api registry cache
        do
            sed -e "s:%SERVICE_TENANT_NAME%:${SERVICE_TENANT_NAME}:g" \
                -i ${GLANCE_CONF_DIR}/glance-$file.conf
            sed -e "s:%SERVICE_USER%:${SRCNAME}:g" -i ${GLANCE_CONF_DIR}/glance-$file.conf
            sed -e "s:%SERVICE_PASSWORD%:${SERVICE_PASSWORD}:g" \
                -i ${GLANCE_CONF_DIR}/glance-$file.conf
            sed -e "s:%DB_PASSWORD%:${DB_PASSWORD}:g" \
                -i ${GLANCE_CONF_DIR}/glance-$file.conf
            sed -e "s:%DB_USER%:${DB_USER}:g" \
                -i ${GLANCE_CONF_DIR}/glance-$file.conf
        done
    fi

    if ${@base_contains('DISTRO_FEATURES', 'sysvinit', 'true', 'false', d)}; then
        install -d ${D}${sysconfdir}/init.d
        sed 's:@suffix@:api:' < ${WORKDIR}/glance.init >${WORKDIR}/glance-api.init.sh
        install -m 0755 ${WORKDIR}/glance-api.init.sh ${D}${sysconfdir}/init.d/glance-api
        sed 's:@suffix@:registry:' < ${WORKDIR}/glance.init >${WORKDIR}/glance-registry.init.sh
        install -m 0755 ${WORKDIR}/glance-registry.init.sh ${D}${sysconfdir}/init.d/glance-registry
    fi

    cp run_tests.sh ${GLANCE_CONF_DIR}
}

CHEF_SERVICES_CONF_FILES := "\
    ${sysconfdir}/${SRCNAME}/glance-api.conf \
    ${sysconfdir}/${SRCNAME}/glance-cache.conf \
    ${sysconfdir}/${SRCNAME}/glance-registry.conf \
    "

pkg_postinst_${SRCNAME}-setup () {
    if [ "x$D" != "x" ]; then
        exit 1
    fi
    
    # This is to make sure postgres is configured and running
    if ! pidof postmaster > /dev/null; then
       /etc/init.d/postgresql-init
       /etc/init.d/postgresql start
       sleep 5
    fi
    
    mkdir /var/log/glance
    sudo -u postgres createdb glance
    glance-manage db_sync
}

PACKAGES += " ${SRCNAME}-tests ${SRCNAME} ${SRCNAME}-setup ${SRCNAME}-api ${SRCNAME}-registry"
ALLOW_EMPTY_${SRCNAME}-setup = "1"

FILES_${PN} = "${libdir}/*"

FILES_${SRCNAME}-tests = "${sysconfdir}/${SRCNAME}/run_tests.sh"

FILES_${SRCNAME} = "${bindir}/* \
    ${sysconfdir}/${SRCNAME}/* \
    ${localstatedir}/* \
    "

FILES_${SRCNAME}-api = "${bindir}/glance-api \
    ${sysconfdir}/init.d/glance-api \
    "

FILES_${SRCNAME}-registry = "${bindir}/glance-registry \
    ${sysconfdir}/init.d/glance-registry \
    "

DEPENDS += " \
        python-pip \
        python-pbr \
        "

RDEPENDS_${PN} += "python-greenlet \
	python-sqlalchemy \
	python-anyjson \
	python-eventlet \
	python-pastedeploy \
	python-routes \
	python-webob \
	python-boto \
	python-sqlalchemy-migrate \
	python-httplib2 \
	python-kombu \
	python-iso8601 \
	python-oslo.config \
	python-pip \
	python-lxml \
	python-paste \
	python-pycrypto \
	python-jsonschema \
	python-keystoneclient \
	python-swiftclient \
	python-pbr \
	python-i18n \
	python-oslo.i18n \
	python-osprofiler \
	python-retrying \
        python-glancestore \
        python-oslo.vmware \
	python-glancestore \
	python-enum34 \
	python-semantic-version \
	"

RDEPENDS_${SRCNAME} = "${PN} \
        postgresql postgresql-client python-psycopg2"
RDEPENDS_${SRCNAME}-api = "${SRCNAME}"
RDEPENDS_${SRCNAME}-registry = "${SRCNAME}"
RDEPENDS_${SRCNAME}-setup = "postgresql sudo ${SRCNAME}"
RDEPENDS_${SRCNAME}-tests = "python-psutil qpid-python bash"

INITSCRIPT_PACKAGES = "${SRCNAME}-api ${SRCNAME}-registry"
INITSCRIPT_NAME_${SRCNAME}-api = "glance-api"
INITSCRIPT_PARAMS_${SRCNAME}-api = "${OS_DEFAULT_INITSCRIPT_PARAMS}"
INITSCRIPT_NAME_${SRCNAME}-registry = "glance-registry"
INITSCRIPT_PARAMS_${SRCNAME}-registry = "${OS_DEFAULT_INITSCRIPT_PARAMS}"

MONITOR_SERVICE_PACKAGES = "${SRCNAME}"
MONITOR_SERVICE_${SRCNAME} = "glance"
