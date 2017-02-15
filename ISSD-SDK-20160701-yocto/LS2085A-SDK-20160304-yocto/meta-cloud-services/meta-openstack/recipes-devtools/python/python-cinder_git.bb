DESCRIPTION = "OpenStack Block storage service"
HOMEPAGE = "https://launchpad.net/cinder"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=1dece7821bf3fd70fe1309eaa37d52a2"

SRCNAME = "cinder"

SRC_URI = "git://github.com/openstack/${SRCNAME}.git;branch=stable/kilo \
    file://cinder.conf \
    file://cinder.init \
    file://cinder-volume \
    file://nfs_setup.sh \
    file://glusterfs_setup.sh \
    file://lvm_iscsi_setup.sh \
    file://add-cinder-volume-types.sh \
    file://cinder-builtin-tests-config-location.patch \
    "

# file://0001-run_tests-respect-tools-dir.patch
# file://fix_cinder_memory_leak.patch 

SRCREV="1ec74b88ac5438c5eba09d64759445574aa97e72"
PV="2015.1.2+git${SRCPV}"
S = "${WORKDIR}/git"

inherit setuptools update-rc.d identity default_configs hosts openstackchef monitor

CINDER_BACKUP_BACKEND_DRIVER ?= "cinder.backup.drivers.swift"

SERVICECREATE_PACKAGES = "${SRCNAME}-setup"
KEYSTONE_HOST="${CONTROLLER_IP}"

# USERCREATE_PARAM and SERVICECREATE_PARAM contain the list of parameters to be set.
# If the flag for a parameter in the list is not set here, the default value will be given to that parameter.
# Parameters not in the list will be set to empty.

USERCREATE_PARAM_${SRCNAME}-setup = "name pass tenant role email"
SERVICECREATE_PARAM_${SRCNAME}-setup = "name type description region publicurl adminurl internalurl"
python () {
    flags = {'type':'volume',\
             'description':'OpenStack Volume Service',\
             'publicurl':"'http://${KEYSTONE_HOST}:8776/v1/\$(tenant_id)s'",\
             'adminurl':"'http://${KEYSTONE_HOST}:8776/v1/\$(tenant_id)s'",\
             'internalurl':"'http://${KEYSTONE_HOST}:8776/v1/\$(tenant_id)s'"}

    d.setVarFlags("SERVICECREATE_PARAM_%s-setup" % d.getVar('SRCNAME',True), flags)
}
SERVICECREATE_PACKAGES[vardeps] += "KEYSTONE_HOST"

do_install_prepend() {
    sed 's:%PYTHON_SITEPACKAGES_DIR%:${PYTHON_SITEPACKAGES_DIR}:g' -i ${S}/${SRCNAME}/tests/conf_fixture.py
}

CINDER_LVM_VOLUME_BACKING_FILE_SIZE ?= "2G"
CINDER_NFS_VOLUME_SERVERS_DEFAULT = "controller:/etc/cinder/nfs_volumes"
CINDER_NFS_VOLUME_SERVERS ?= "${CINDER_NFS_VOLUME_SERVERS_DEFAULT}"
CINDER_GLUSTERFS_VOLUME_SERVERS_DEFAULT = "controller:/glusterfs_volumes"
CINDER_GLUSTERFS_VOLUME_SERVERS ?= "${CINDER_GLUSTERFS_VOLUME_SERVERS_DEFAULT}"

do_install_append() {
    TEMPLATE_CONF_DIR=${S}${sysconfdir}/${SRCNAME}
    CINDER_CONF_DIR=${D}${sysconfdir}/${SRCNAME}
    
    #Instead of substituting api-paste.ini from the WORKDIR,
    #move it over to the image's directory and do the substitution there
    install -d ${CINDER_CONF_DIR}
    install -m 600 ${WORKDIR}/cinder.conf ${CINDER_CONF_DIR}/
    install -m 600 ${TEMPLATE_CONF_DIR}/api-paste.ini ${CINDER_CONF_DIR}/
    install -m 600 ${S}/etc/cinder/policy.json ${CINDER_CONF_DIR}/

    install -d ${CINDER_CONF_DIR}/drivers
    install -m 600 ${WORKDIR}/nfs_setup.sh ${CINDER_CONF_DIR}/drivers/
    install -m 600 ${WORKDIR}/glusterfs_setup.sh ${CINDER_CONF_DIR}/drivers/
    install -m 600 ${WORKDIR}/lvm_iscsi_setup.sh ${CINDER_CONF_DIR}/drivers/
    install -m 700 ${WORKDIR}/add-cinder-volume-types.sh ${CINDER_CONF_DIR}/

    install -d ${D}${localstatedir}/log/${SRCNAME}
    
    if [ -z "${OPENSTACKCHEF_ENABLED}" ]; then
        for file in api-paste.ini cinder.conf; do
        sed -e "s:%SERVICE_TENANT_NAME%:${SERVICE_TENANT_NAME}:g" \
            -i ${CINDER_CONF_DIR}/$file
        sed -e "s:%SERVICE_USER%:${SRCNAME}:g" -i ${CINDER_CONF_DIR}/$file
        sed -e "s:%SERVICE_PASSWORD%:${SERVICE_PASSWORD}:g" \
            -i ${CINDER_CONF_DIR}/$file

        sed -e "s:%DB_USER%:${DB_USER}:g" -i ${CINDER_CONF_DIR}/$file
        sed -e "s:%DB_PASSWORD%:${DB_PASSWORD}:g" -i ${CINDER_CONF_DIR}/$file
        sed -e "s:%CINDER_BACKUP_BACKEND_DRIVER%:${CINDER_BACKUP_BACKEND_DRIVER}:g" \
        -i ${CINDER_CONF_DIR}/$file
        done
    fi

    if ${@base_contains('DISTRO_FEATURES', 'sysvinit', 'true', 'false', d)}; then
        install -d ${D}${sysconfdir}/init.d
        sed 's:@suffix@:api:' < ${WORKDIR}/cinder.init >${WORKDIR}/cinder-api.init.sh
        install -m 0755 ${WORKDIR}/cinder-api.init.sh ${D}${sysconfdir}/init.d/cinder-api
        sed 's:@suffix@:scheduler:' < ${WORKDIR}/cinder.init >${WORKDIR}/cinder-scheduler.init.sh
        install -m 0755 ${WORKDIR}/cinder-scheduler.init.sh ${D}${sysconfdir}/init.d/cinder-scheduler
        sed 's:@suffix@:backup:' < ${WORKDIR}/cinder.init >${WORKDIR}/cinder-backup.init.sh
        install -m 0755 ${WORKDIR}/cinder-backup.init.sh ${D}${sysconfdir}/init.d/cinder-backup
        install -m 0755 ${WORKDIR}/cinder-volume ${D}${sysconfdir}/init.d/cinder-volume
    fi

    # test setup
    cp run_tests.sh ${CINDER_CONF_DIR}
    cp -r tools ${CINDER_CONF_DIR}

    #Create cinder volume group backing file
    sed 's/%CINDER_LVM_VOLUME_BACKING_FILE_SIZE%/${CINDER_LVM_VOLUME_BACKING_FILE_SIZE}/g' -i ${D}/etc/cinder/drivers/lvm_iscsi_setup.sh
    mkdir -p ${D}/etc/tgt/
    echo "include /etc/cinder/data/volumes/*" > ${D}/etc/tgt/targets.conf

    # Create Cinder nfs_share config file with default nfs server
    echo "${CINDER_NFS_VOLUME_SERVERS}" > ${D}/etc/cinder/nfs_shares
    sed 's/\s\+/\n/g' -i ${D}/etc/cinder/nfs_shares
    [ "x${CINDER_NFS_VOLUME_SERVERS}" = "x${CINDER_NFS_VOLUME_SERVERS_DEFAULT}" ] && is_default="1" || is_default="0"
    sed -e "s:%IS_DEFAULT%:${is_default}:g" -i ${D}/etc/cinder/drivers/nfs_setup.sh

    # Create Cinder glusterfs_share config file with default glusterfs server
    echo "${CINDER_GLUSTERFS_VOLUME_SERVERS}" > ${D}/etc/cinder/glusterfs_shares
    sed 's/\s\+/\n/g' -i ${D}/etc/cinder/glusterfs_shares
    [ "x${CINDER_GLUSTERFS_VOLUME_SERVERS}" = "x${CINDER_GLUSTERFS_VOLUME_SERVERS_DEFAULT}" ] && is_default="1" || is_default="0"
    sed -e "s:%IS_DEFAULT%:${is_default}:g" -i ${D}/etc/cinder/drivers/glusterfs_setup.sh
}

CHEF_SERVICES_CONF_FILES :="\
    ${sysconfdir}/${SRCNAME}/cinder.conf \
    ${sysconfdir}/${SRCNAME}/api-paste.ini \
    "
pkg_postinst_${SRCNAME}-setup () {
    if [ "x$D" != "x" ]; then
        exit 1
    fi

    # This is to make sure postgres is configured and running
    if ! pidof postmaster > /dev/null; then
       /etc/init.d/postgresql-init
       /etc/init.d/postgresql start
    fi

    if [ ! -d /var/log/cinder ]; then
       mkdir /var/log/cinder
    fi

    sudo -u postgres createdb cinder
    cinder-manage db sync

    # Create Cinder nfs_share config file with default nfs server
    if [ ! -f /etc/cinder/nfs_shares ]; then
        /bin/bash /etc/cinder/drivers/nfs_setup.sh
    fi

    # Create Cinder glusterfs_share config file with default glusterfs server
    if [ ! -f /etc/cinder/glusterfs_shares ] && [ -f /usr/sbin/glusterfsd ]; then
        /bin/bash /etc/cinder/drivers/glusterfs_setup.sh
    fi
}

PACKAGES += "${SRCNAME}-tests ${SRCNAME} ${SRCNAME}-setup ${SRCNAME}-api ${SRCNAME}-volume ${SRCNAME}-scheduler ${SRCNAME}-backup"
ALLOW_EMPTY_${SRCNAME}-setup = "1"

RDEPENDS_${SRCNAME}-tests += " bash"

FILES_${PN} = "${libdir}/* /etc/tgt"

FILES_${SRCNAME}-tests = "${sysconfdir}/${SRCNAME}/run_tests.sh \
                          ${sysconfdir}/${SRCNAME}/tools"

FILES_${SRCNAME}-api = "${bindir}/cinder-api \
    ${sysconfdir}/init.d/cinder-api"

FILES_${SRCNAME}-volume = "${bindir}/cinder-volume \
    ${sysconfdir}/init.d/cinder-volume"

FILES_${SRCNAME}-scheduler = "${bindir}/cinder-scheduler \
    ${sysconfdir}/init.d/cinder-scheduler"

FILES_${SRCNAME}-backup = "${bindir}/cinder-backup \
    ${sysconfdir}/init.d/cinder-backup"

FILES_${SRCNAME} = "${bindir}/* \
    ${sysconfdir}/${SRCNAME}/* \
    ${localstatedir}/* \
    ${sysconfdir}/${SRCNAME}/drivers/* \
    "

DEPENDS += " \
        python-pip \
        python-pbr \
        "

RDEPENDS_${PN} += "lvm2 \
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
	python-cinderclient \
	python-oslo.config \
	python-oslo.rootwrap \
	python-pbr \
	python-taskflow \
	python-rtslib-fb \
	"

RDEPENDS_${SRCNAME} = "${PN} \
        postgresql postgresql-client python-psycopg2 tgt"

RDEPENDS_${SRCNAME}-api = "${SRCNAME}"
RDEPENDS_${SRCNAME}-volume = "${SRCNAME}"
RDEPENDS_${SRCNAME}-scheduler = "${SRCNAME}"
RDEPENDS_${SRCNAME}-setup = "postgresql sudo ${SRCNAME}"

INITSCRIPT_PACKAGES = "${SRCNAME}-api ${SRCNAME}-volume ${SRCNAME}-scheduler ${SRCNAME}-backup"
INITSCRIPT_NAME_${SRCNAME}-api = "cinder-api"
INITSCRIPT_PARAMS_${SRCNAME}-api = "${OS_DEFAULT_INITSCRIPT_PARAMS}"
INITSCRIPT_NAME_${SRCNAME}-volume = "cinder-volume"
INITSCRIPT_PARAMS_${SRCNAME}-volume = "${OS_DEFAULT_INITSCRIPT_PARAMS}"
INITSCRIPT_NAME_${SRCNAME}-scheduler = "cinder-scheduler"
INITSCRIPT_PARAMS_${SRCNAME}-scheduler = "${OS_DEFAULT_INITSCRIPT_PARAMS}"
INITSCRIPT_NAME_${SRCNAME}-backup = "cinder-backup"
INITSCRIPT_PARAMS_${SRCNAME}-backup = "${OS_DEFAULT_INITSCRIPT_PARAMS}"

MONITOR_SERVICE_PACKAGES = "${SRCNAME}"
MONITOR_SERVICE_${SRCNAME} = "cinder"
