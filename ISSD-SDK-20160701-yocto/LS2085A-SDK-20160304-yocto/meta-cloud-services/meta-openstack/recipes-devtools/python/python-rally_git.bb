DESCRIPTION = "Rally - OpenStack benchmarking at a scale - is intended to provide the \
community with a benchmarking tool that is capable of performing specific, \
complicated and reproducible test cases on real deployment scenarios."
HOMEPAGE = "https://launchpad.net/rally"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=19cbd64715b51267a47bf3750cc6a8a5"

PR = "r0"
SRCNAME = "rally"

SRC_URI = "git://github.com/stackforge/${SRCNAME}.git;branch=master \
           file://rally.init \
           file://rally.conf \
           file://task-example.json \
           file://deployment-existing.json \
           file://remove-ironic-support.patch \
           file://verification-to-use-existing-tempest.patch \
           file://sqlalchemy-db-missing-name-for-ENUM.patch \
           file://verification-subunit2json-fail-to-open-result-file.patch \
"

SRCREV="b297cf00750f263b8b5bdeb71f6952f672e87f5a"
PV="git${SRCPV}"
S = "${WORKDIR}/git"

inherit setuptools update-rc.d hosts identity default_configs

do_install_append() {
    RALLY_CONF_DIR=${D}${sysconfdir}/${SRCNAME}
    RALLY_PYTHON_SITEPACKAGES_DIR=${D}${PYTHON_SITEPACKAGES_DIR}/${SRCNAME}

    install -d ${RALLY_CONF_DIR}
    install -m 600 ${WORKDIR}/rally.conf ${RALLY_CONF_DIR}/rally.conf
    sed -e "s:%DB_USER%:${DB_USER}:g" -i ${RALLY_CONF_DIR}/rally.conf
    sed -e "s:%DB_PASSWORD%:${DB_PASSWORD}:g" -i ${RALLY_CONF_DIR}/rally.conf

    if ${@base_contains('DISTRO_FEATURES', 'sysvinit', 'true', 'false', d)}; then
        install -d ${D}${sysconfdir}/init.d
        sed 's:@suffix@:api:' < ${WORKDIR}/rally.init > ${WORKDIR}/rally-api.init.sh
        install -m 0755 ${WORKDIR}/rally-api.init.sh ${D}${sysconfdir}/init.d/rally-api
    fi

    install -d ${RALLY_PYTHON_SITEPACKAGES_DIR}
    cp -r ${S}/tests* ${RALLY_PYTHON_SITEPACKAGES_DIR}
    find ${RALLY_PYTHON_SITEPACKAGES_DIR}/tests* -name "*.py" | xargs \
            sed -i 's:^from tests:from rally.tests:g'
    install -d ${RALLY_PYTHON_SITEPACKAGES_DIR}/doc/samples
    cp -r ${S}/doc/samples/* ${RALLY_PYTHON_SITEPACKAGES_DIR}/doc/samples

    install -d ${RALLY_CONF_DIR}/tasks
    install -m 600 ${WORKDIR}/task-example.json ${RALLY_CONF_DIR}/tasks/example.json

    install -d ${RALLY_CONF_DIR}/deployments
    install -m 600 ${WORKDIR}/deployment-existing.json ${RALLY_CONF_DIR}/deployments/existing.json
    sed -e "s:%ADMIN_TENANT_NAME%:admin:g" -i ${RALLY_CONF_DIR}/deployments/existing.json
    sed -e "s:%ADMIN_USER%:admin:g" -i ${RALLY_CONF_DIR}/deployments/existing.json
    sed -e "s:%ADMIN_PASSWORD%:${ADMIN_PASSWORD}:g" -i ${RALLY_CONF_DIR}/deployments/existing.json
    sed -e "s:%CONTROLLER_IP%:${CONTROLLER_IP}:g" -i ${RALLY_CONF_DIR}/deployments/existing.json

    cp ${S}/run_tests.sh ${RALLY_CONF_DIR}
    cp -r ${S}/tools ${RALLY_CONF_DIR}
}

pkg_postinst_${SRCNAME}-setup () {
    if [ "x$D" != "x" ]; then
        exit 1
    fi

    # This is to make sure postgres is configured and running
    if ! pidof postmaster > /dev/null; then
       /etc/init.d/postgresql-init
       /etc/init.d/postgresql start
    fi

    if [ ! -d /var/log/rally ]; then
       mkdir /var/log/rally
    fi

    sudo -u postgres createdb rally
    rally-manage db recreate
}

PACKAGES += "${SRCNAME}-tests ${SRCNAME}-api ${SRCNAME} ${SRCNAME}-setup"
ALLOW_EMPTY_${SRCNAME}-setup = "1"

FILES_${PN} = "${libdir}/*"

FILES_${SRCNAME}-tests = "${sysconfdir}/${SRCNAME}/run_tests.sh \
    "

FILES_${SRCNAME} = "${bindir}/* \
    ${sysconfdir}/${SRCNAME}/* \
    "

FILES_${SRCNAME}-api = "${bindir}/rally-api \
    ${sysconfdir}/init.d/rally-api \
    "

DEPENDS += " \
    python-pip \
    python-pbr \
    "

RDEPENDS_${PN} += " python-babel \
    python-decorator \
    python-fixtures \
    python-iso8601 \
    python-jsonschema \
    python-netaddr \
    python-oslo.config \
    python-paramiko \
    python-pbr \
    python-pecan \
    python-prettytable \
    python-pyyaml \
    python-glanceclient \
    python-keystoneclient \
    python-novaclient \
    python-neutronclient \
    python-cinderclient \
    python-heatclient \
    python-ceilometerclient \
    python-subunit \
    python-requests \
    python-sqlalchemy \
    python-six \
    python-wsme \
    "

RDEPENDS_${SRCNAME}-tests = "${PN} \
   python-coverage \
   python-mock \
   python-testrepository \
   python-testtools \
   python-oslotest \
   "

RDEPENDS_${SRCNAME} = "${PN} \
   postgresql \
   postgresql-client \
   "

RDEPENDS_${SRCNAME}-setup = "postgresql sudo ${SRCNAME}"
RDEPENDS_${SRCNAME}-api = "${SRCNAME}"

INITSCRIPT_PACKAGES = "${SRCNAME}-api"
INITSCRIPT_NAME_${SRCNAME}-api = "${SRCNAME}-api"
INITSCRIPT_PARAMS_${SRCNAME}-api = "${OS_DEFAULT_INITSCRIPT_PARAMS}"
