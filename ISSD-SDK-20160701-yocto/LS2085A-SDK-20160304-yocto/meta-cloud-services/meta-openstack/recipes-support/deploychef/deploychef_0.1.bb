#
# Copyright (C) 2014 Wind River Systems, Inc.
#
SUMMARY = "For the provisioning of OpenStack nodes"
DESCRIPTION = "There are a number of variables that are baked into Openstack \
at build time, for example the ip address of a compute or controller node. \
This means that when a new compute or controller node boots up, it will \
have an ip address that differs from its currently assigned ip address \
This package facilitates the recreation of openstack script and \ 
configuration files, as well as their placement in the appropriate directories on \
the files system on a compute/controller/allinone node at runtime"

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COREBASE}/LICENSE;md5=4d92cd373abda3937c2bc47fbc49d690 \
                    file://${COREBASE}/meta/COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"

PR = "r1"


RDEPENDS_${PN} = "chef"
SRC_URI = "\
    file://deploychef.init  \
    file://attributes.json  \
    file://config.rb \
    file://default_recipe.rb \
    file://run-openstackchef \
    file://run-postinsts \
    file://run-deploychef \
    file://service-shutdown \
    file://deploychef-inc \
    "
inherit update-rc.d identity hosts default_configs

S = "${WORKDIR}"
#Since this package does not need to be ran for each boot-up
#There is no need for an init scrpt so install it in /opt/${BPN}
DEPLOYCHEF_ROOT_DIR ?= "/opt/${BPN}"
POSTINSTS_DIR ?= "rpm-postinsts"

#Provide a mechanism for these strings to be over-written if necessary
COOKBOOK_DIR = "${DEPLOYCHEF_ROOT_DIR}/cookbooks/"
ATTRIBUTE_DIR = "${DEPLOYCHEF_ROOT_DIR}/cookbooks/openstack/attributes/"
RECIPE_DIR = "${DEPLOYCHEF_ROOT_DIR}/cookbooks/openstack/recipes/"

FILES_${PN} += " \
    ${DEPLOYCHEF_ROOT_DIR}/* \
    ${DEPLOYCHEF_ROOT_DIR}/conf-templates/* \
    ${DEPLOYCHEF_ROOT_DIR}/cookbooks/* \
    ${DEPLOYCHEF_ROOT_DIR}/cookbooks/openstack/recipes/* \
    ${DEPLOYCHEF_ROOT_DIR}/cookbooks/openstack/templates/* \
    ${DEPLOYCHEF_ROOT_DIR}/cookbooks/openstack/templates/default \
    ${DEPLOYCHEF_ROOT_DIR}/cookbooks/openstack/attributes \
    ${DEPLOYCHEF_ROOT_DIR}/cookbooks/openstack/attributes/* \
    "
#Read the module config files and make them into
#chef-solo templates
do_install() {
    if ${@base_contains('DISTRO_FEATURES', 'sysvinit', 'true', 'false', d)}; then
        #This script will make templates out of postinst script before they
        #have a chance to run
        install -d ${D}${sysconfdir}/init.d
        install -m 0755 ${S}/${BPN}.init ${D}${sysconfdir}/init.d/${BPN}

        install -d ${D}/${DEPLOYCHEF_ROOT_DIR}
        #Copy the template configuration scripts to image directory
        install -m 0644 ${S}/config.rb ${D}/${DEPLOYCHEF_ROOT_DIR}/config.rb
        install -m 0644 ${S}/attributes.json ${D}/${DEPLOYCHEF_ROOT_DIR}/attributes.json
        install -m 0755 ${S}/run-postinsts ${D}/${DEPLOYCHEF_ROOT_DIR}/run-postinsts
        install -m 0755 ${S}/run-openstackchef ${D}/${DEPLOYCHEF_ROOT_DIR}/run-openstackchef
        install -m 0755 ${S}/run-deploychef  ${D}/${DEPLOYCHEF_ROOT_DIR}/run-deploychef
        install -m 0755 ${S}/service-shutdown  ${D}/${DEPLOYCHEF_ROOT_DIR}/service-shutdown
        install -m 0644 ${S}/deploychef-inc  ${D}/${DEPLOYCHEF_ROOT_DIR}/deploychef-inc
        #Copy the chefsolo recipe file to chefsolo recipe folder
        install -d ${D}/${RECIPE_DIR}
        install -m 0644 ${S}/default_recipe.rb  ${D}/${RECIPE_DIR}/default.rb
    fi
}

do_install_append() {

    #Replace all required placeholders
    for file in "${D}/${DEPLOYCHEF_ROOT_DIR}/run-deploychef \ 
        ${D}/${DEPLOYCHEF_ROOT_DIR}/service-shutdown \
        ${D}/${DEPLOYCHEF_ROOT_DIR}/deploychef-inc \
        ${D}/${DEPLOYCHEF_ROOT_DIR}/run-postinsts \
        ${D}/${DEPLOYCHEF_ROOT_DIR}/run-openstackchef \
        ${D}/${RECIPE_DIR}/default.rb \
        ${D}/${sysconfdir}/init.d/${BPN} "; do

        sed -i s:%SYSCONFDIR%:${sysconfdir}:g $file
        sed -i s:%POSTINSTS_DIR%:${POSTINSTS_DIR}:g $file
        sed -i s:%PACKAGE_NAME%:${BPN}:g $file
        sed -i s:%DEPLOYCHEF_ROOT_DIR%:${DEPLOYCHEF_ROOT_DIR}:g $file
    done
}

INITSCRIPT_PACKAGES = "${BPN}"
INITSCRIPT_NAME_${BPN} = "${BPN}"
INITSCRIPT_PARAMS = "start 96 S ."

