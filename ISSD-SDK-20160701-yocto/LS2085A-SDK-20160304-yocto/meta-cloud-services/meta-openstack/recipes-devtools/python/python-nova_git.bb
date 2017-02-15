DESCRIPTION = "Nova is a cloud computing fabric controller"
HOMEPAGE = "https://launchpad.net/nova"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=1dece7821bf3fd70fe1309eaa37d52a2"

DEPENDS = "sudo libvirt"

PR = "r0"
SRCNAME = "nova"

FILESEXTRAPATHS_append := "${THISDIR}/${PN}"

SRC_URI = "git://github.com/openstack/${SRCNAME}.git;branch=stable/kilo \
           file://nova-add-migrate.cfg-to-the-MANIFEST.patch \
           "
# restore post stable/juno:
#           file://neutron-api-set-default-binding-vnic_type.patch \
#           file://websocketproxy-allow-empty-schemes-at-python-2.7.3.patch
#           file://nova-convert-path-from-relative-to-absolute.patch 
#           file://nova-fix-location-to-doc-directory.patch
#           file://nova-fix-location-to-plugin-directory.patch 

SRC_URI += "file://nova-all \
            file://nova.init \
            file://nova-consoleauth \
            file://nova.conf \
            file://openrc \
           "
SRCREV="24c6b904f75b6a544edc9d34ca0163df7d164a1a"
# 2015.1.2 is 24c6b904f75b6a544edc9d34ca0163df7d164a1a
PV="2015.1.2+git${SRCPV}"

S = "${WORKDIR}/git"

inherit update-rc.d setuptools identity hosts useradd default_configs openstackchef monitor

LIBVIRT_IMAGES_TYPE ?= "default"

SERVICECREATE_PACKAGES = "${SRCNAME}-setup ${SRCNAME}-ec2"
KEYSTONE_HOST="${CONTROLLER_IP}"

# USERCREATE_PARAM and SERVICECREATE_PARAM contain the list of parameters to be set.
# If the flag for a parameter in the list is not set here, the default value will be given to that parameter.
# Parameters not in the list will be set to empty.

USERCREATE_PARAM_${SRCNAME}-setup = "name pass tenant role email"
SERVICECREATE_PARAM_${SRCNAME}-setup = "name type description region publicurl adminurl internalurl"
python () {
    flags = {'type':'compute',\
             'description':'OpenStack Compute Service',\
             'publicurl':"'http://${KEYSTONE_HOST}:8774/v2/\$(tenant_id)s'",\
             'adminurl':"'http://${KEYSTONE_HOST}:8774/v2/\$(tenant_id)s'",\
             'internalurl':"'http://${KEYSTONE_HOST}:8774/v2/\$(tenant_id)s'"}
    d.setVarFlags("SERVICECREATE_PARAM_%s-setup" % d.getVar('SRCNAME',True), flags)
}

# ec2 is provided by nova-api
SERVICECREATE_PARAM_${SRCNAME}-ec2 = "name type description region publicurl adminurl internalurl"
python () {
    flags = {'name':'ec2',\
             'type':'ec2',\
             'description':'OpenStack EC2 Service',\
             'publicurl':"'http://${KEYSTONE_HOST}:8773/services/Cloud'",\
             'adminurl':"'http://${KEYSTONE_HOST}:8773/services/Admin'",\
             'internalurl':"'http://${KEYSTONE_HOST}:8773/services/Cloud'"}
    d.setVarFlags("SERVICECREATE_PARAM_%s-ec2" % d.getVar('SRCNAME',True), flags)
}

do_install_append() {
    if [ ! -f "${WORKDIR}/nova.conf" ]; then
        return
    fi

    TEMPLATE_CONF_DIR=${S}${sysconfdir}/${SRCNAME}
    NOVA_CONF_DIR=${D}/${sysconfdir}/nova

    install -d ${NOVA_CONF_DIR}
    install -o nova -m 664 ${S}/etc/nova/policy.json ${NOVA_CONF_DIR}/

    # Deploy filters to /etc/nova/rootwrap.d
    install -m 755 -d ${NOVA_CONF_DIR}/rootwrap.d
    install -m 600 ${S}/etc/nova/rootwrap.d/*.filters ${NOVA_CONF_DIR}/rootwrap.d
    chown -R root:root ${NOVA_CONF_DIR}/rootwrap.d
    chmod 644 ${NOVA_CONF_DIR}/rootwrap.d

    # Set up rootwrap.conf, pointing to /etc/nova/rootwrap.d
    install -m 644 ${S}/etc/nova/rootwrap.conf ${NOVA_CONF_DIR}/
    sed -e "s:^filters_path=.*$:filters_path=${sysconfdir}/nova/rootwrap.d:" \
        -i ${NOVA_CONF_DIR}/rootwrap.conf
    chown root:root $NOVA_CONF_DIR/rootwrap.conf

    # Set up the rootwrap sudoers for nova
    install -d ${D}${sysconfdir}/sudoers.d
    touch ${D}${sysconfdir}/sudoers.d/nova-rootwrap
    chmod 0440 ${D}${sysconfdir}/sudoers.d/nova-rootwrap
    chown root:root ${D}${sysconfdir}/sudoers.d/nova-rootwrap
    # root user setup
    echo "root ALL=(root) NOPASSWD: ${bindir}/nova-rootwrap" > \
        ${D}${sysconfdir}/sudoers.d/nova-rootwrap
    # nova user setup
    echo "nova ALL=(root) NOPASSWD: ${bindir}/nova-rootwrap ${sysconfdir}/nova/rootwrap.conf *" >> \
         ${D}${sysconfdir}/sudoers.d/nova-rootwrap

    # Copy the configuration file
    install -o nova -m 664 ${WORKDIR}/nova.conf               ${NOVA_CONF_DIR}/nova.conf
    install -o nova -m 664 ${TEMPLATE_CONF_DIR}/api-paste.ini ${NOVA_CONF_DIR}
    install -o nova -m 664 ${WORKDIR}/openrc                  ${NOVA_CONF_DIR}
    if [ -z "${OPENSTACKCHEF_ENABLED}" ]; then
        # Configuration options
        sed -e "s:%SERVICE_TENANT_NAME%:${SERVICE_TENANT_NAME}:g" \
            -i ${NOVA_CONF_DIR}/api-paste.ini
        sed -e "s:%SERVICE_USER%:${SRCNAME}:g" -i ${NOVA_CONF_DIR}/api-paste.ini
        sed -e "s:%SERVICE_PASSWORD%:${SERVICE_PASSWORD}:g" \
            -i ${NOVA_CONF_DIR}/api-paste.ini
        sed -e "s:%CONTROLLER_IP%:${CONTROLLER_IP}:g" -i ${NOVA_CONF_DIR}/api-paste.ini

        sed -e "s:%DB_USER%:${DB_USER}:g" -i ${NOVA_CONF_DIR}/nova.conf
        sed -e "s:%DB_PASSWORD%:${DB_PASSWORD}:g" -i ${NOVA_CONF_DIR}/nova.conf

        sed -e "s:%METADATA_SHARED_SECRET%:${METADATA_SHARED_SECRET}:g" -i ${NOVA_CONF_DIR}/nova.conf

        sed -e "s:%CONTROLLER_IP%:${CONTROLLER_IP}:g" -i ${NOVA_CONF_DIR}/nova.conf
        sed -e "s:%CONTROLLER_HOST%:${CONTROLLER_HOST}:g" -i ${NOVA_CONF_DIR}/nova.conf

        sed -e "s:%COMPUTE_IP%:${COMPUTE_IP}:g" -i ${NOVA_CONF_DIR}/nova.conf
        sed -e "s:%COMPUTE_HOST%:${COMPUTE_HOST}:g" -i ${NOVA_CONF_DIR}/nova.conf

        sed -e "s:%SERVICE_TENANT_NAME%:${SERVICE_TENANT_NAME}:g" -i ${NOVA_CONF_DIR}/nova.conf
        sed -e "s:%SERVICE_USER%:${SRCNAME}:g" -i ${NOVA_CONF_DIR}/nova.conf
        sed -e "s:%SERVICE_PASSWORD%:${SERVICE_PASSWORD}:g" -i ${NOVA_CONF_DIR}/nova.conf

        sed -e "s:%LIBVIRT_IMAGES_TYPE%:${LIBVIRT_IMAGES_TYPE}:g" -i ${NOVA_CONF_DIR}/nova.conf

        sed -e "s:%OS_PASSWORD%:${ADMIN_PASSWORD}:g" -i ${NOVA_CONF_DIR}/openrc
        sed -e "s:%SERVICE_TOKEN%:${SERVICE_TOKEN}:g" -i ${NOVA_CONF_DIR}/openrc

        sed -e "s:%CONTROLLER_IP%:${CONTROLLER_IP}:g" -i ${NOVA_CONF_DIR}/openrc
        sed -e "s:%CONTROLLER_HOST%:${CONTROLLER_HOST}:g" -i ${NOVA_CONF_DIR}/openrc
    fi
    install -o nova -d ${NOVA_CONF_DIR}/instances

    if ${@base_contains('DISTRO_FEATURES', 'sysvinit', 'true', 'false', d)}; then
        install -d ${D}${sysconfdir}/init.d

	# nova-all is installed (and packaged), but not used as an initscript by default
        install -m 0755 ${WORKDIR}/nova-all ${D}${sysconfdir}/init.d/nova-all
        install -m 0755 ${WORKDIR}/nova-consoleauth ${D}${sysconfdir}/init.d/nova-consoleauth

	for binary in api compute network scheduler cert conductor novncproxy spicehtml5proxy; do
	    sed "s:@suffix@:$binary:" < ${WORKDIR}/nova.init >${WORKDIR}/nova-$binary.init.sh
            install -m 0755 ${WORKDIR}/nova-$binary.init.sh ${D}${sysconfdir}/init.d/nova-$binary
	done	
    fi

    cp run_tests.sh ${NOVA_CONF_DIR}

    install -d ${D}/${sysconfdir}/bash_completion.d
    install -m 664 ${S}/tools/nova-manage.bash_completion ${D}/${sysconfdir}/bash_completion.d

    cp -r "${S}/doc" "${D}/${PYTHON_SITEPACKAGES_DIR}/nova"
    cp -r "${S}/plugins" "${D}/${PYTHON_SITEPACKAGES_DIR}/nova"
}

CHEF_SERVICES_CONF_FILES := "\
    ${sysconfdir}/${SRCNAME}/nova.conf \
    ${sysconfdir}/${SRCNAME}/api-paste.ini \
    ${sysconfdir}/${SRCNAME}/openrc \
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

    sudo -u postgres createdb nova
    sleep 2
    nova-manage db sync
}

pkg_postinst_${SRCNAME}-common () {
    if [ "x$D" != "x" ]; then
        exit 1
    fi

    if [ -d  /home/root ]; then
        echo "source /etc/nova/openrc" >> /home/root/.bashrc
	echo "source /etc/nova/openrc" >> /home/root/.profile
    else
        echo "source /etc/nova/openrc" >> /root/.bashrc
	echo "source /etc/nova/openrc" >> /root/.profile
    fi
}

USERADD_PACKAGES = "${PN}"
GROUPADD_PARAM_${PN} = "--system nova"
USERADD_PARAM_${PN}  = "--system --home /var/lib/nova -g nova -G libvirt \
                        --no-create-home --shell /bin/false nova"

PACKAGES += " ${SRCNAME}-tests"
PACKAGES += " ${SRCNAME}-setup ${SRCNAME}-common ${SRCNAME}-compute ${SRCNAME}-controller"
PACKAGES += " ${SRCNAME}-consoleauth"
PACKAGES += " ${SRCNAME}-novncproxy"
PACKAGES += " ${SRCNAME}-spicehtml5proxy"
PACKAGES += " ${SRCNAME}-network"
PACKAGES += " ${SRCNAME}-scheduler"
PACKAGES += " ${SRCNAME}-cert"
PACKAGES += " ${SRCNAME}-conductor"
PACKAGES += " ${SRCNAME}-api"
PACKAGES += " ${SRCNAME}-ec2"

PACKAGECONFIG ?= "bash-completion"
PACKAGECONFIG[bash-completion] = ",,bash-completion,bash-completion python-nova-bash-completion"

PACKAGES =+ "${BPN}-bash-completion"
FILES_${BPN}-bash-completion = "${sysconfdir}/bash_completion.d/*"


ALLOW_EMPTY_${SRCNAME}-setup = "1"
ALLOW_EMPTY_${SRCNAME}-ec2 = "1"

FILES_${PN} = "${libdir}/*"

FILES_${SRCNAME}-tests = "${sysconfdir}/${SRCNAME}/run_tests.sh"

FILES_${SRCNAME}-common = "${bindir}/nova-manage \
                           ${bindir}/nova-rootwrap \
                           ${sysconfdir}/${SRCNAME}/* \
                           ${sysconfdir}/sudoers.d"

FILES_${SRCNAME}-compute = "${bindir}/nova-compute \
                            ${sysconfdir}/init.d/nova-compute"

FILES_${SRCNAME}-controller = "${bindir}/* \
 			       ${sysconfdir}/init.d/nova-all "

FILES_${SRCNAME}-consoleauth = " \
	${sysconfdir}/init.d/nova-consoleauth \
"
FILES_${SRCNAME}-novncproxy = " \
	${sysconfdir}/init.d/nova-novncproxy \
"
FILES_${SRCNAME}-spicehtml5proxy = " \
	${sysconfdir}/init.d/nova-spicehtml5proxy \
"
FILES_${SRCNAME}-network = " \
	${sysconfdir}/init.d/nova-network \
"
FILES_${SRCNAME}-scheduler = " \
	${sysconfdir}/init.d/nova-scheduler \
"
FILES_${SRCNAME}-cert = " \
	${sysconfdir}/init.d/nova-cert \
"
FILES_${SRCNAME}-conductor = " \
	${sysconfdir}/init.d/nova-conductor \
"
FILES_${SRCNAME}-api = " \
	${sysconfdir}/init.d/nova-api \
"

DEPENDS += " \
        libvirt \
        python-pip \
        python-pbr \
        "

RDEPENDS_${PN} = " libvirt \
		   python-keystone \
		   python-keystonemiddleware \
		   python-modules \
		   python-misc \
		   python-amqp \
		   python-amqplib \
		   python-anyjson \
		   python-babel \
		   python-boto \
		   python-novaclient \
		   python-cinderclient \
		   python-cliff \
		   python-cheetah \
		   python-eventlet \
		   python-feedparser \
		   python-glanceclient \
		   python-greenlet \
		   python-httplib2 \
		   python-iso8601 \
		   python-jinja2 \
		   python-kombu \
		   python-lxml \
		   python-netaddr \
		   python-oslo.config \
		   python-oslo.rootwrap \
		   python-oslo.concurrency \
		   python-oslo.middleware \
		   python-oslo.context \
		   python-oslo.log \
		   python-paste \
		   python-pastedeploy \
		   python-paramiko \
		   python-psutil \
		   python-pyasn1 \
		   python-setuptools-git \
		   python-simplejson \
		   python-jsonschema \
		   python-six \
		   python-setuptools \
		   python-sqlalchemy \
		   python-sqlalchemy-migrate \
		   python-stevedore \
		   python-suds \
		   python-neutronclient \
		   python-routes \
		   python-webob \
		   python-websockify \
		   python-pbr \
		   spice-html5 \
		   python-posix-ipc \
		   python-rfc3986 \
		   python-oslo.i18n \
		   python-psutil \
		   python-sqlparse \
    "

RDEPENDS_${SRCNAME}-common = "${PN} openssl openssl-misc libxml2 libxslt \
                              iptables curl dnsmasq sudo procps"

RDEPENDS_${SRCNAME}-controller = "${PN} ${SRCNAME}-common \
				  ${SRCNAME}-ec2 \
				  ${SRCNAME}-consoleauth \
				  ${SRCNAME}-novncproxy \
				  ${SRCNAME}-spicehtml5proxy \
				  ${SRCNAME}-network \
				  ${SRCNAME}-scheduler \
				  ${SRCNAME}-cert \
				  ${SRCNAME}-conductor \
                                  ${SRCNAME}-api \
				  postgresql postgresql-client python-psycopg2"

RDEPENDS_${SRCNAME}-compute = "${PN} ${SRCNAME}-common python-oslo.messaging \
			       qemu libvirt libvirt-libvirtd libvirt-python libvirt-virsh \
			       util-linux-blkid \
			       "
RDEPENDS_${SRCNAME}-setup = "postgresql sudo ${SRCNAME}-common"
RDEPENDS_${SRCNAME}-ec2 = "postgresql sudo ${SRCNAME}-common"

RDEPENDS_${SRCNAME}-tests = " \
                            python-coverage \
                            bash \
                            "

INITSCRIPT_PACKAGES =  "${SRCNAME}-compute ${SRCNAME}-consoleauth ${SRCNAME}-novncproxy ${SRCNAME}-spicehtml5proxy"
INITSCRIPT_PACKAGES += "${SRCNAME}-network ${SRCNAME}-scheduler ${SRCNAME}-cert ${SRCNAME}-conductor"
INITSCRIPT_PACKAGES += "${SRCNAME}-api"

# nova-all can replace: network, scheduler, cert, conductor and api. 
# by default we go for the more granular initscripts, but this is left
# in case nova-all is desired.
# INITSCRIPT_PACKAGES += "${SRCNAME}-controller"
# INITSCRIPT_NAME_${SRCNAME}-controller = "nova-all"
INITSCRIPT_NAME_${SRCNAME}-network = "nova-network"
INITSCRIPT_PARAMS_${SRCNAME}-network = "${OS_DEFAULT_INITSCRIPT_PARAMS}"
INITSCRIPT_NAME_${SRCNAME}-scheduler = "nova-scheduler"
INITSCRIPT_PARAMS_${SRCNAME}-scheduler = "${OS_DEFAULT_INITSCRIPT_PARAMS}"
INITSCRIPT_NAME_${SRCNAME}-cert = "nova-cert"
INITSCRIPT_PARAMS_${SRCNAME}-cert = "${OS_DEFAULT_INITSCRIPT_PARAMS}"
INITSCRIPT_NAME_${SRCNAME}-conductor = "nova-conductor"
INITSCRIPT_PARAMS_${SRCNAME}-conductor = "${OS_DEFAULT_INITSCRIPT_PARAMS}"
INITSCRIPT_NAME_${SRCNAME}-api = "nova-api"
INITSCRIPT_PARAMS_${SRCNAME}-api = "${OS_DEFAULT_INITSCRIPT_PARAMS}"

INITSCRIPT_NAME_${SRCNAME}-compute = "nova-compute"
INITSCRIPT_PARAMS_${SRCNAME}-compute = "${OS_DEFAULT_INITSCRIPT_PARAMS}"
INITSCRIPT_NAME_${SRCNAME}-consoleauth = "nova-consoleauth"
INITSCRIPT_PARAMS_${SRCNAME}-consoleauth = "${OS_DEFAULT_INITSCRIPT_PARAMS}"
INITSCRIPT_NAME_${SRCNAME}-novncproxy = "nova-novncproxy"
INITSCRIPT_PARAMS_${SRCNAME}-novncproxy = "${OS_DEFAULT_INITSCRIPT_PARAMS}"

INITSCRIPT_NAME_${SRCNAME}-spicehtml5proxy = "nova-spicehtml5proxy"
INITSCRIPT_PARAMS_${SRCNAME}-spicehtml5proxy = "${OS_DEFAULT_INITSCRIPT_PARAMS}"

MONITOR_SERVICE_PACKAGES = "${SRCNAME}"
MONITOR_SERVICE_${SRCNAME} = "nova-api nova-cert nova-conductor nova-consoleauth nova-scheduler"
