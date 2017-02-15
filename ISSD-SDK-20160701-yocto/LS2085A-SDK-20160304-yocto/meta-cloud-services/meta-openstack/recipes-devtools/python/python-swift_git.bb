DESCRIPTION = "Highly available, distributed, eventually consistent object/blob store."
HOMEPAGE = "https://launchpad.net/swift"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=3b83ef96387f14655fc854ddc3c6bd57"

SRCNAME = "swift"

SRC_URI = "git://github.com/openstack/${SRCNAME}.git;branch=stable/kilo \
           file://proxy-server.conf \
           file://dispersion.conf \
           file://test.conf \
           file://swift.init \
           file://swift_setup.sh \
           file://cluster.conf \
"

SRCREV="f8dee761bd36f857aa1288c27e095907032fad68"
PV="2.3.0+git${SRCPV}"
S = "${WORKDIR}/git"

inherit setuptools python-dir update-rc.d hosts identity openstackchef

# The size of the backing file (in Gigabytes) of loopback devices
# which are used for setting up Swift storage devices.  The value
# of "0G" is for disabling loopback devices and using local
# filesystem as Swift storage device.  In this case, the local
# filesystem of must support xattrs. e.g ext4
SWIFT_BACKING_FILE_SIZE ?= "2G"

SERVICECREATE_PACKAGES = "${SRCNAME}-setup"
KEYSTONE_HOST="${CONTROLLER_IP}"

# USERCREATE_PARAM and SERVICECREATE_PARAM contain the list of parameters to be set.
# If the flag for a parameter in the list is not set here, the default value will be given to that parameter.
# Parameters not in the list will be set to empty.

USERCREATE_PARAM_${SRCNAME}-setup = "name pass tenant role email"
SERVICECREATE_PARAM_${SRCNAME}-setup = "name type description region publicurl adminurl internalurl"
python () {
    flags = {'type':'object-store',\
             'description':'OpenStack object-store',\
             'publicurl':"'http://${KEYSTONE_HOST}:8888/v1/AUTH_\$(tenant_id)s'",\
             'adminurl':"'http://${KEYSTONE_HOST}:8888/v1'",\
             'internalurl':"'http://${KEYSTONE_HOST}:8888/v1/AUTH_\$(tenant_id)s'"}

    d.setVarFlags("SERVICECREATE_PARAM_%s-setup" % d.getVar('SRCNAME',True), flags)
}

do_install_append() {
    SWIFT_CONF_DIR=${D}${sysconfdir}/swift

    install -d ${SWIFT_CONF_DIR}
    install -d ${D}${sysconfdir}/init.d/

    install -m 600 ${S}/etc/swift.conf-sample ${SWIFT_CONF_DIR}/swift.conf
    install -m 600 ${WORKDIR}/proxy-server.conf ${SWIFT_CONF_DIR}/proxy-server.conf
    install -m 600 ${S}/etc/account-server.conf-sample ${SWIFT_CONF_DIR}/account-server.conf
    install -m 600 ${S}/etc/container-server.conf-sample ${SWIFT_CONF_DIR}/container-server.conf
    install -m 600 ${S}/etc/object-server.conf-sample ${SWIFT_CONF_DIR}/object-server.conf
    install -m 600 ${WORKDIR}/dispersion.conf ${SWIFT_CONF_DIR}/dispersion.conf
    install -m 600 ${WORKDIR}/test.conf ${SWIFT_CONF_DIR}/test.conf
    install -m 755 ${WORKDIR}/swift.init ${D}${sysconfdir}/init.d/swift
    install -m 700 ${WORKDIR}/swift_setup.sh ${SWIFT_CONF_DIR}/swift_setup.sh
    install -m 600 ${WORKDIR}/cluster.conf ${SWIFT_CONF_DIR}/cluster.conf

    sed 's/^# bind_port =.*/bind_port = 6002/' -i ${SWIFT_CONF_DIR}/account-server.conf
    sed 's/^# user =.*/user = root/' -i ${SWIFT_CONF_DIR}/account-server.conf
    sed 's/^# swift_dir =.*/swift_dir = \/etc\/swift/' -i ${SWIFT_CONF_DIR}/account-server.conf
    sed 's/^# devices =.*/devices = \/etc\/swift\/node/' -i ${SWIFT_CONF_DIR}/account-server.conf
    sed 's/^# mount_check =.*/mount_check = false/' -i ${SWIFT_CONF_DIR}/account-server.conf

    sed 's/^# bind_port =.*/bind_port = 6001/' -i ${SWIFT_CONF_DIR}/container-server.conf
    sed 's/^# user =.*/user = root/' -i ${SWIFT_CONF_DIR}/container-server.conf
    sed 's/^# swift_dir =.*/swift_dir = \/etc\/swift/' -i ${SWIFT_CONF_DIR}/container-server.conf
    sed 's/^# devices =.*/devices = \/etc\/swift\/node/' -i ${SWIFT_CONF_DIR}/container-server.conf
    sed 's/^# mount_check =.*/mount_check = false/' -i ${SWIFT_CONF_DIR}/container-server.conf

    sed 's/^# bind_port =.*/bind_port = 6000/' -i ${SWIFT_CONF_DIR}/object-server.conf
    sed 's/^# user =.*/user = root/' -i ${SWIFT_CONF_DIR}/object-server.conf
    sed 's/^# swift_dir =.*/swift_dir = \/etc\/swift/' -i ${SWIFT_CONF_DIR}/object-server.conf
    sed 's/^# devices =.*/devices = \/etc\/swift\/node/' -i ${SWIFT_CONF_DIR}/object-server.conf
    sed 's/^# mount_check =.*/mount_check = false/' -i ${SWIFT_CONF_DIR}/object-server.conf
    if [ -z "${OPENSTACKCHEF_ENABLED}" ]; then
        sed "s/%SERVICE_TENANT_NAME%/${SERVICE_TENANT_NAME}/g" -i ${SWIFT_CONF_DIR}/proxy-server.conf
        sed "s/%SERVICE_USER%/${SRCNAME}/g" -i ${SWIFT_CONF_DIR}/proxy-server.conf
        sed "s/%SERVICE_PASSWORD%/${SERVICE_PASSWORD}/g" -i ${SWIFT_CONF_DIR}/proxy-server.conf

        sed "s/%SERVICE_TENANT_NAME%/${SERVICE_TENANT_NAME}/g" -i ${SWIFT_CONF_DIR}/dispersion.conf
        sed "s/%SERVICE_USER%/${SRCNAME}/g" -i ${SWIFT_CONF_DIR}/dispersion.conf
        sed "s/%SERVICE_PASSWORD%/${SERVICE_PASSWORD}/g" -i ${SWIFT_CONF_DIR}/dispersion.conf

        sed "s/%ADMIN_TENANT_NAME%/admin/g" -i ${SWIFT_CONF_DIR}/test.conf
        sed "s/%ADMIN_USER%/admin/g" -i ${SWIFT_CONF_DIR}/test.conf
        sed "s/%ADMIN_PASSWORD%/${ADMIN_PASSWORD}/g" -i ${SWIFT_CONF_DIR}/test.conf
        sed "s/%SERVICE_TENANT_NAME%/${SERVICE_TENANT_NAME}/g" -i ${SWIFT_CONF_DIR}/test.conf
        sed "s/%SERVICE_USER%/${SRCNAME}/g" -i ${SWIFT_CONF_DIR}/test.conf
        sed "s/%SERVICE_PASSWORD%/${SERVICE_PASSWORD}/g" -i ${SWIFT_CONF_DIR}/test.conf
        sed "s/%DEMO_USER%/demo/g" -i ${SWIFT_CONF_DIR}/test.conf
        sed "s/%DEMO_PASSWORD%/${ADMIN_PASSWORD}/g" -i ${SWIFT_CONF_DIR}/test.conf

        sed "s/%SWIFT_BACKING_FILE_SIZE%/${SWIFT_BACKING_FILE_SIZE}/g" -i ${D}${sysconfdir}/init.d/swift
        sed "s/%CONTROLLER_IP%/${CONTROLLER_IP}/g" -i ${D}${sysconfdir}/init.d/swift
    fi
    cp -r test ${D}/${PYTHON_SITEPACKAGES_DIR}/${SRCNAME}/
    grep -rl '^from test' ${D}/${PYTHON_SITEPACKAGES_DIR}/${SRCNAME}/test | xargs sed 's/^from test/from swift\.test/g' -i

}

CHEF_SERVICES_CONF_FILES := " \
    ${sysconfdir}/${SRCNAME}/test.conf \
    ${sysconfdir}/${SRCNAME}/dispersion.conf \
    ${sysconfdir}/${SRCNAME}/proxy-server.conf \
    ${sysconfdir}/init.d/swift \
    "

pkg_postinst_${SRCNAME}-setup () {
    if [ "x$D" != "x" ]; then
        exit 1
    fi

    CLUSTER_CONF=/etc/swift/cluster.conf
    SWIFT_SETUP='/bin/bash /etc/swift/swift_setup.sh'

    for i in `seq 1 3`; do
        BACKING_FILE=/etc/swift/swift_backing_$i
        if [ "x${SWIFT_BACKING_FILE_SIZE}" != "x0G" ]; then
            truncate -s ${SWIFT_BACKING_FILE_SIZE} $BACKING_FILE
            sed "s:%SWIFT_BACKING_FILE_${i}%:$BACKING_FILE:g" -i $CLUSTER_CONF
        else
            sed "s:%SWIFT_BACKING_FILE_${i}%::g" -i $CLUSTER_CONF
        fi
    done

    $SWIFT_SETUP createrings
    $SWIFT_SETUP formatdevs
    $SWIFT_SETUP mountdevs
    $SWIFT_SETUP -i "${CONTROLLER_IP}" adddevs
    $SWIFT_SETUP unmountdevs
}

PACKAGES += "${SRCNAME}-tests ${SRCNAME} ${SRCNAME}-setup"

FILES_${PN} = "${libdir}/*\
"

FILES_${SRCNAME}-tests = "${sysconfdir}/${SRCNAME}/test.conf \
"

FILES_${SRCNAME}-setup = "${sysconfdir}/init.d/swift \
    ${sysconfdir}/${SRCNAME}/swift_setup.sh \
    ${sysconfdir}/${SRCNAME}/cluster.conf \
"

FILES_${SRCNAME} = "${bindir}/* \
    ${sysconfdir}/${SRCNAME}/* \
"

DEPENDS += " \
        python-pip \
        python-pbr \
        "

RDEPENDS_${PN} += " \
    python-eventlet \
    python-greenlet \
    python-pastedeploy \
    python-simplejson \
    python-swiftclient \
    python-netifaces \
    python-xattr \
    python-pbr \
    python-dnspython \
    bash \
    "

RDEPENDS_${SRCNAME} = "${PN}"

RDEPENDS_${SRCNAME} = "${PN}"
RDEPENDS_${SRCNAME}-setup = "${SRCNAME}"

INITSCRIPT_PACKAGES = "${SRCNAME}-setup"
INITSCRIPT_NAME_${SRCNAME}-setup = "swift"
INITSCRIPT_PARAMS_${SRCNAME}-setup = "defaults"
