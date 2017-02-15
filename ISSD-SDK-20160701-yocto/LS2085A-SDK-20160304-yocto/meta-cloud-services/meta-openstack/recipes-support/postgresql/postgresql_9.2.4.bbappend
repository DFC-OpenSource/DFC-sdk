FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://postgresql \
            file://postgresql-init"

inherit useradd update-rc.d identity hosts autotools-brokensep openstackchef

PACKAGECONFIG[libxml] = "--with-libxml CFLAGS=-I${STAGING_INCDIR}/libxml2,--without-libxml,libxml2,libxml2"

# default
DB_DATADIR ?= "/var/lib/postgres/data"

do_install_append() {
    INIT_D_DEST_DIR=${D}${sysconfdir}/init.d

    install -d ${D}${sysconfdir}/init.d/
    install -m 0755 ${WORKDIR}/postgresql ${INIT_D_DEST_DIR}/postgresql
    install -m 0755 ${WORKDIR}/postgresql-init ${INIT_D_DEST_DIR}/postgresql-init
    if [ -z "${OPENSTACKCHEF_ENABLED}" ]; then
        sed -e "s:%DB_DATADIR%:${DB_DATADIR}:g" -i ${INIT_D_DEST_DIR}/postgresql
        sed -e "s:%DB_DATADIR%:${DB_DATADIR}:g" -i ${INIT_D_DEST_DIR}/postgresql-init

        sed -e "s:%DB_USER%:${DB_USER}:g" -i ${INIT_D_DEST_DIR}/postgresql-init
        sed -e "s:%DB_PASSWORD%:${DB_PASSWORD}:g" -i ${INIT_D_DEST_DIR}/postgresql-init

        sed -e "s:%CONTROLLER_IP%:${CONTROLLER_IP}:g" -i ${INIT_D_DEST_DIR}/postgresql-init
        sed -e "s:%CONTROLLER_HOST%:${CONTROLLER_HOST}:g" -i ${INIT_D_DEST_DIR}/postgresql-init

        sed -e "s:%COMPUTE_IP%:${COMPUTE_IP}:g" -i ${INIT_D_DEST_DIR}/postgresql-init
        sed -e "s:%COMPUTE_HOST%:${COMPUTE_HOST}:g" -i ${INIT_D_DEST_DIR}/postgresql-init
    fi
}

CHEF_SERVICES_CONF_FILES := "\
    ${sysconfdir}/init.d/postgresql \
    ${sysconfdir}/init.d/postgresql-init \
    "

RDEPENDS_${PN} += "postgresql-timezone eglibc-utils update-rc.d"
USERADD_PACKAGES = "${PN}"
GROUPADD_PARAM_${PN} = "--system postgres"
USERADD_PARAM_${PN}  = "--system --home /var/lib/postgres -g postgres \
                        --no-create-home --shell /bin/false postgres"

PACKAGES += " ${PN}-setup"
ALLOW_EMPTY_${PN}-setup = "1"

pkg_postinst_${PN}-setup () {
    # postgres 9.2.4 postinst
    if [ "x$D" != "x" ]; then
        exit 1
    fi
      
    /etc/init.d/postgresql-init
    if [ $? -ne 0 ]; then
        echo "[ERROR] postgres: unable to create admin account"
        exit 1
    fi
}

FILES_${PN} += "${localstatedir}/run/${PN}"

INITSCRIPT_PACKAGES = "${PN}"
INITSCRIPT_NAME = "${PN}"
INITSCRIPT_PARAMS = "defaults"
