DESCRIPTION = "The OpenStack Dashboard."
HOMEPAGE = "https://github.com/openstack/horizon/"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=1dece7821bf3fd70fe1309eaa37d52a2"

DEPENDS += " \
        python-pip \
        python-pbr \
        "

RDEPENDS_${PN} += " python-django \
    python-django-appconf \
    python-django-compressor \
    python-django-nose \
    python-django-openstack-auth \
    python-netaddr \
    python-ceilometerclient \
    python-cinderclient \
    python-glanceclient \
    python-heatclient \
    python-keystoneclient \
    python-troveclient \
    python-saharaclient \
    python-novaclient \
    python-nose-exclude \
    python-neutronclient \
    python-pytz \
    python-pbr \
    python-six \
    python-swiftclient \
    python-pyyaml \
    python-pint \
    python-xstatic \
    python-xstatic-jquery \
    python-xstatic-angular \
    python-xstatic-angular-cookies \
    python-xstatic-angular-mock \
    python-xstatic-angular-bootstrap \
    python-xstatic-angular-irdragndrop \
    python-xstatic-angular-lrdragndrop \
    python-xstatic-magic-search \
    python-xstatic-d3 \
    python-xstatic-hogan \
    python-xstatic-jasmine \
    python-xstatic-jquery-migrate \
    python-xstatic-jquery-quicksearch \
    python-xstatic-jquery-tablesorter \
    python-xstatic-jsencrypt \
    python-xstatic-qunit \
    python-xstatic-rickshaw \
    python-xstatic-spin \
    python-xstatic-bootstrap-datepicker \
    python-xstatic-bootstrap-scss \
    python-xstatic-font-awesome \
    python-xstatic-jquery-ui \
    python-xstatic-smart-table \
    python-xstatic-term.js \
    python-pyscss \
    python-django-pyscss \
    "

SRCNAME = "horizon"

SRC_URI = "git://github.com/openstack/${SRCNAME}.git;branch=stable/kilo \
    file://horizon.init \
    file://fix_bindir_path.patch \
    file://openstack-dashboard-apache.conf \
    file://local_settings.py \
    file://horizon-use-full-package-path-to-test-directories.patch \
    "

SRCREV = "bccc3dcba65275e099137c8ca37f1077e87120d5"
PV = "2015.1.2+git${SRCPV}"
S = "${WORKDIR}/git"

inherit setuptools update-rc.d python-dir default_configs openstackchef monitor

# no longer required. kept as reference.
# do_install[dirs] += "${D}/usr/share/bin"

do_install_append() {
    SYSCONF_DIR=${D}${sysconfdir}
    DASHBOARD_CONF_DIR=${SYSCONF_DIR}/openstack-dashboard
    DASHBOARD_SHARE_DIR=${D}${datadir}/openstack-dashboard
    HORIZON_CONF_DIR=${D}${sysconfdir}/horizon

    install -d ${HORIZON_CONF_DIR}

    DASHBOARD_DIR=${D}${PYTHON_SITEPACKAGES_DIR}/openstack_dashboard
    sed -e "s:^LANGUAGE_CODE =.*:LANGUAGE_CODE = 'en-us':g" \
        -i ${DASHBOARD_DIR}/settings.py
    sed -e "s:^# from horizon.utils:from horizon.utils:g" \
        ${DASHBOARD_DIR}/local/local_settings.py.example > \
        ${DASHBOARD_DIR}/local/local_settings.py
    sed -e "s:^# SECRET_KEY =:SECRET_KEY =:g" \
        -i ${DASHBOARD_DIR}/local/local_settings.py
    install -m 644 ${S}/manage.py ${DASHBOARD_DIR}/manage.py

    if ${@base_contains('DISTRO_FEATURES', 'sysvinit', 'true', 'false', d)};
    then
        install -d ${D}${sysconfdir}/init.d
        sed 's:@PYTHON_SITEPACKAGES@:${PYTHON_SITEPACKAGES_DIR}:' \
            ${WORKDIR}/horizon.init >${WORKDIR}/horizon
        install -m 0755 ${WORKDIR}/horizon ${D}${sysconfdir}/init.d/horizon
    fi
    sed -i -e 's#%PYTHON_SITEPACKAGES%#${PYTHON_SITEPACKAGES_DIR}#g' \
        ${D}${PYTHON_SITEPACKAGES_DIR}/horizon/test/settings.py

    # no longer required. kept as reference.
    # mv ${D}${datadir}/bin ${DASHBOARD_DIR}/bin

    cp run_tests.sh ${HORIZON_CONF_DIR}

    # the following are setup required for horizon-apache
    install -d ${DASHBOARD_SHARE_DIR}
    cp -a ${S}/openstack_dashboard  ${DASHBOARD_SHARE_DIR}
    cp ${S}/manage.py  ${DASHBOARD_SHARE_DIR}

    install -D -m 644 ${WORKDIR}/local_settings.py \
        ${DASHBOARD_CONF_DIR}/local_settings.py
    ln -fs ${sysconfdir}/openstack-dashboard/local_settings.py \
        ${DASHBOARD_SHARE_DIR}/openstack_dashboard/local/local_settings.py

    install -D -m 644 ${WORKDIR}/openstack-dashboard-apache.conf \
      ${SYSCONF_DIR}/apache2/conf.d/openstack-dashboard-apache.conf
    sed -i -e 's#%PYTHON_SITEPACKAGES%#${PYTHON_SITEPACKAGES_DIR}#' \
        ${SYSCONF_DIR}/apache2/conf.d/openstack-dashboard-apache.conf
    sed -i -e 's#%LIBDIR%#${libdir}#' \
        ${SYSCONF_DIR}/apache2/conf.d/openstack-dashboard-apache.conf

    ln -fs openstack_dashboard/static ${DASHBOARD_SHARE_DIR}/static

    # daemon is UID 1
    chown -R 1 ${DASHBOARD_SHARE_DIR}/openstack_dashboard/static
}

PACKAGES += "${SRCNAME}-tests ${SRCNAME} ${SRCNAME}-apache ${SRCNAME}-standalone"

RDEPENDS_${SRCNAME}-tests += " bash"

FILES_${PN} = "${libdir}/*"

FILES_${SRCNAME}-tests = "${sysconfdir}/${SRCNAME}/run_tests.sh"

FILES_${SRCNAME} = "${bindir}/* \
    ${datadir}/* \
    "

FILES_${SRCNAME}-standalone = "${sysconfdir}/init.d/horizon"

FILES_${SRCNAME}-apache = " \
    ${sysconfdir}/apache2 \
    ${sysconfdir}/openstack-dashboard/ \
    ${datadir}/openstack-dashboard/ \
    "

RDEP_ARCH_VAR = ""
RDEP_ARCH_VAR_arm = "nodejs"
RDEP_ARCH_VAR_i686 = "nodejs"
RDEP_ARCH_VAR_x86-64 = "nodejs"
RDEP_ARCH_VAR_ia32 = "nodejs"

RDEPENDS_${PN} += " \
    ${RDEP_ARCH_VAR} \
    "

RDEPENDS_${SRCNAME} = "${PN}"

INITSCRIPT_PACKAGES = "${SRCNAME}"
INITSCRIPT_NAME_${SRCNAME} = "horizon"
INITSCRIPT_PARAMS_${SRCNAME} = "${OS_DEFAULT_INITSCRIPT_PARAMS}"

RDEPENDS_${SRCNAME}-apache = "\
    apache2 \
    mod-wsgi \
    memcached \
    python-memcached \
    "

MONITOR_SERVICE_PACKAGES = "${SRCNAME}"
MONITOR_SERVICE_${SRCNAME} = "horizon"

CLEANBROKEN = "1"

