DESCRIPTION = "The OpenStack Integration Test Suite"
HOMEPAGE = "https://launchpad.net/tempest"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=1dece7821bf3fd70fe1309eaa37d52a2"

PR = "r0"
SRCNAME = "tempest"

inherit setuptools identity hosts

SRC_URI = "git://github.com/openstack/${SRCNAME}.git;branch=master \
           file://tempest.conf \
           file://logging.conf \
"

SRCREV="2707a0f065e52d8331d12c983ead95de1224cb32"
PV="2014.1+git${SRCPV}"
S = "${WORKDIR}/git"

SERVICECREATE_PACKAGES = "${SRCNAME}-setup ${SRCNAME}-setup-altdemo ${SRCNAME}-setup-admin"
KEYSTONE_HOST="${CONTROLLER_IP}"

# USERCREATE_PARAM and SERVICECREATE_PARAM contain the list of parameters to be set.
# If the flag for a parameter in the list is not set here, the default value will be given to that parameter.
# Parameters not in the list will be set to empty.

# create demo user
USERCREATE_PARAM_${SRCNAME}-setup = "name pass tenant role email"
python () {
    flags = {'name':'demo',\
             'pass':'password',\
             'tenant':'demo',\
             'role':'${MEMBER_ROLE}',\
             'email':'demo@domain.com',\
            }
    d.setVarFlags("USERCREATE_PARAM_%s-setup" % d.getVar('SRCNAME',True), flags)
}

# create alt-demo user
USERCREATE_PARAM_${SRCNAME}-setup-altdemo = "name pass tenant role email"
python () {
    flags = {'name':'alt_demo',\
             'pass':'password',\
             'tenant':'alt_demo',\
             'role':'${MEMBER_ROLE}',\
             'email':'alt_demo@domain.com',\
            }
    d.setVarFlags("USERCREATE_PARAM_%s-setup-altdemo" % d.getVar('SRCNAME',True), flags)
}

# add admin user to demo tenant as admin role
USERCREATE_PARAM_${SRCNAME}-setup-admin = "name pass tenant role email"
python () {
    flags = {'name':'${ADMIN_USER}',\
             'pass':'${ADMIN_PASSWORD}',\
             'tenant':'demo',\
             'role':'${ADMIN_ROLE}',\
             'email':'${ADMIN_USER_EMAIL}',\
            }
    d.setVarFlags("USERCREATE_PARAM_%s-setup-admin" % d.getVar('SRCNAME',True), flags)
}

do_install_append() {
    TEMPLATE_CONF_DIR=${S}${sysconfdir}/
    TEMPEST_CONF_DIR=${D}${sysconfdir}/${SRCNAME}

    sed -e "s:%SERVICE_TENANT_NAME%:${SERVICE_TENANT_NAME}:g" -i ${WORKDIR}/tempest.conf
    sed -e "s:%SERVICE_USER%:${SRCNAME}:g" -i ${WORKDIR}/tempest.conf
    sed -e "s:%SERVICE_PASSWORD%:${SERVICE_PASSWORD}:g" -i ${WORKDIR}/tempest.conf

    sed -e "s:%DB_USER%:${DB_USER}:g" -i ${WORKDIR}/tempest.conf
    sed -e "s:%DB_PASSWORD%:${DB_PASSWORD}:g" -i ${WORKDIR}/tempest.conf

    sed -e "s:%CONTROLLER_IP%:${CONTROLLER_IP}:g" -i ${WORKDIR}/tempest.conf
    sed -e "s:%CONTROLLER_HOST%:${CONTROLLER_HOST}:g" -i ${WORKDIR}/tempest.conf

    sed -e "s:%COMPUTE_IP%:${COMPUTE_IP}:g" -i ${WORKDIR}/tempest.conf
    sed -e "s:%COMPUTE_HOST%:${COMPUTE_HOST}:g" -i ${WORKDIR}/tempest.conf

    sed -e "s:%ADMIN_PASSWORD%:${ADMIN_PASSWORD}:g" -i ${WORKDIR}/tempest.conf
    sed -e "s:%SERVICE_TENANT_NAME%:${SERVICE_TENANT_NAME}:g" -i ${WORKDIR}/tempest.conf

    install -d ${TEMPEST_CONF_DIR}
    install -d ${TEMPEST_CONF_DIR}/tests
    install -m 600 ${WORKDIR}/tempest.conf ${TEMPEST_CONF_DIR}
    install -m 600 ${WORKDIR}/logging.conf ${TEMPEST_CONF_DIR}
    install -m 600 ${TEMPLATE_CONF_DIR}/*.yaml ${TEMPEST_CONF_DIR}

    # relocate tests to somewhere less cryptic, which means we pull them out of
    # site-packages and put them in /etc/tempest/tests/
    for t in api cli scenario stress thirdparty; do
       ln -s ${PYTHON_SITEPACKAGES_DIR}/${SRCNAME}/$t ${TEMPEST_CONF_DIR}/tests/
    done

    # test infrastructure
    cp run_tests.sh ${TEMPEST_CONF_DIR}
    cp .testr.conf ${TEMPEST_CONF_DIR}
    sed "s:discover -t ./ ./tempest:discover -t ${PYTHON_SITEPACKAGES_DIR} tempest:g" -i ${TEMPEST_CONF_DIR}/.testr.conf
    cp -r tools ${TEMPEST_CONF_DIR}
}

PACKAGES =+ "${SRCNAME}-tests \
             ${SRCNAME}-setup \
             ${SRCNAME}-setup-altdemo \
             ${SRCNAME}-setup-admin \
             "

FILES_${SRCNAME}-tests = "${sysconfdir}/${SRCNAME}/tests/*"

FILES_${PN} = "${libdir}/* \
               ${sysconfdir}/* \
               ${bindir}/* \
"

ALLOW_EMPTY_${SRCNAME}-setup = "1"
ALLOW_EMPTY_${SRCNAME}-setup-altdemo = "1"
ALLOW_EMPTY_${SRCNAME}-setup-admin = "1"

RDEPENDS_${PN} += " \
        ${SRCNAME}-tests \
        ${SRCNAME}-setup \
        ${SRCNAME}-setup-altdemo \
        ${SRCNAME}-setup-admin \
	python-mox \
	python-mock \
	python-hp3parclient \
	python-oauth2 \
	python-testrepository \
	python-fixtures \
	python-keyring \
	python-glanceclient \
	python-keystoneclient \
	python-swiftclient \
	python-novaclient \
	python-cinderclient \
	python-heatclient \
	python-pbr \
	python-anyjson \
	python-nose \
	python-httplib2 \
	python-jsonschema \
	python-testtools \
	python-lxml \
	python-boto \
	python-paramiko \
	python-netaddr \
	python-testresources \
	python-oslo.config \
	python-eventlet \
	python-six \
	python-iso8601 \
	python-mimeparse \
	python-flake8 \
	python-tox \
	"

