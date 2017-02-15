#
# Copyright (C) 2014 Wind River Systems, Inc.
#
# The identity class provides utilities for services to add tenant/role/users,
# and service/endpoints into keystone database
#

SERVICE_TOKEN = "password"
METADATA_SHARED_SECRET = "password"

DB_USER = "admin"
DB_PASSWORD = "admin"

SERVICE_TENANT_NAME = "service"
SERVICE_PASSWORD = "password"

ADMIN_TENANT = "admin"
ADMIN_USER = "admin"
ADMIN_PASSWORD = "password"
ADMIN_ROLE = "admin"
ADMIN_USER_EMAIL = "admin@domain.com"

MEMBER_ROLE = "Member"

RUN_POSTINSTS_FILE = "${@base_contains('DISTRO_FEATURES', 'sysvinit', '/etc/rcS.d/S99run-postinsts', '', d)}"

# Add service and user setup into S99run-postinst running list
ROOTFS_POSTPROCESS_COMMAND += "update_run_postinsts ; "
POST_SERVICE_SETUP_COMMAND = "/etc/keystone/service-user-setup"

update_run_postinsts() {
    if [ -f "${IMAGE_ROOTFS}${RUN_POSTINSTS_FILE}" ]; then
        cat >> "${IMAGE_ROOTFS}${RUN_POSTINSTS_FILE}" << EOF

# run service and user setup
if [ -f ${POST_SERVICE_SETUP_COMMAND} ]; then
    chmod 755 ${POST_SERVICE_SETUP_COMMAND}
    ${POST_SERVICE_SETUP_COMMAND}
fi

# run hybrid backend setup
if [ -f "${POST_KEYSTONE_SETUP_COMMAND}" ]; then
    chmod 755 ${POST_KEYSTONE_SETUP_COMMAND}
    ${POST_KEYSTONE_SETUP_COMMAND}
fi
EOF
    fi
}

# Create user and service in package postinst, common part
servicecreate_postinst_common () {

    # create service and user setup postinstall file
    if [ ! -e ${POST_SERVICE_SETUP_COMMAND} ]; then
        cat > ${POST_SERVICE_SETUP_COMMAND} << EOF
#!/bin/sh
EOF
    fi
}

# Create user in package postinst
servicecreate_postinst_user () {

    # create tenant/user/role in keystone
    cat >> ${POST_SERVICE_SETUP_COMMAND} << EOF

    /etc/keystone/identity.sh user-create USERCREATE_PARAM
EOF
}

# Create service in package postinst
servicecreate_postinst_service () {

    # create service/endpoint in keystone
    cat >> ${POST_SERVICE_SETUP_COMMAND} << EOF

    /etc/keystone/identity.sh service-create SERVICECREATE_PARAM
EOF
}

# Recipe parse-time sanity checks
def sanity_check(d):
    servicecreate_packages = d.getVar('SERVICECREATE_PACKAGES', True) or ""

    for pkg in servicecreate_packages.split():
        # User parameters checking.
        if not d.getVar('USERCREATE_PARAM_%s' % pkg, True) and not d.getVar('SERVICECREATE_PARAM_%s' % pkg, True):
            raise bb.build.FuncFailed, "%s SERVICECREATE_PACKAGES includes %s, but neither USERCREATE_PARAM_%s nor SERVICECREATE_PARAM_%s is set" % (d.getVar('FILE'), pkg, pkg, pkg)

python __anonymous() {
    sanity_check(d)
}

# Get user variables from recipe and return a string that will be passed to identity.sh
def usercreate_param(d, pkg):
    # Default values
    param_defaults = {'name':'${SRCNAME}',\
                      'pass':'${SERVICE_PASSWORD}',\
                      'tenant':'${SERVICE_TENANT_NAME}',\
                      'role':'${ADMIN_ROLE}',\
                      'email':'${SRCNAME}@domain.com'}

    param = d.getVar('USERCREATE_PARAM_%s' % pkg, True)
    param_flags = d.getVarFlags('USERCREATE_PARAM_%s' % pkg) or {}

    for key, value in param_defaults.items():
        if key in param.split():
            if param_flags.has_key(key):
                param_defaults[key] = param_flags[key]
        else:
            param_defaults[key] = ''

    user_param = '--name=' + param_defaults['name'] + ' ' \
               + '--pass=' + param_defaults['pass'] + ' ' \
               + '--tenant=' + param_defaults['tenant'] + ' ' \
               + '--role=' + param_defaults['role'] + ' ' \
               + '--email=' + param_defaults['email']

    bb.debug(1, 'user_param = %s' % user_param)
    return user_param

# Get service variables from recipe and return a string that will be passed to identity.sh
def servicecreate_param(d, pkg):
    # Default values
    param_defaults = {'name':'${SRCNAME}',\
                      'type':'',\
                      'description':'',\
                      'region':'RegionOne',\
                      'publicurl':'',\
                      'adminurl':'',\
                      'internalurl':''}

    param = d.getVar('SERVICECREATE_PARAM_%s' % pkg, True)
    param_flags = d.getVarFlags('SERVICECREATE_PARAM_%s' % pkg) or {}

    for key, value in param_defaults.items():
        if key in param.split():
            if param_flags.has_key(key):
                param_defaults[key] = param_flags[key]
        else:
            param_defaults[key] = ''

    service_param = '--name=' + param_defaults['name'] + ' ' \
                  + '--type=' + param_defaults['type'] + ' ' \
                  + '--description=' + param_defaults['description'] + ' ' \
                  + '--region=' + param_defaults['region'] + ' ' \
                  + '--publicurl=' + param_defaults['publicurl'] + ' ' \
                  + '--adminurl=' + param_defaults['adminurl'] + ' ' \
                  + '--internalurl=' + param_defaults['internalurl']

    bb.debug(1, 'service_param = %s' % service_param)
    return service_param

# Add the postinst script into the generated package
python populate_packages_append () {
    servicecreate_packages = d.getVar('SERVICECREATE_PACKAGES', True) or ""

    servicecreate_postinst_common_copy = d.getVar('servicecreate_postinst_common', True)
    servicecreate_postinst_user_copy = d.getVar('servicecreate_postinst_user', True)
    servicecreate_postinst_service_copy = d.getVar('servicecreate_postinst_service', True)
    for pkg in servicecreate_packages.split():
        bb.debug(1, 'Adding service/user creation calls to postinst for %s' % pkg)

        postinst = d.getVar('pkg_postinst_%s' % pkg, True) or d.getVar('pkg_postinst', True)
        if not postinst:
            postinst = '    if [ "x$D" != "x" ]; then\n' + \
                       '        exit 1\n' + \
                       '    fi\n'
        postinst += servicecreate_postinst_common_copy

        if d.getVar('USERCREATE_PARAM_%s' % pkg, True):
            servicecreate_postinst_user = servicecreate_postinst_user_copy.replace("USERCREATE_PARAM", usercreate_param(d, pkg))
            postinst += servicecreate_postinst_user

        if d.getVar('SERVICECREATE_PARAM_%s' % pkg, True):
            servicecreate_postinst_service = servicecreate_postinst_service_copy.replace("SERVICECREATE_PARAM", servicecreate_param(d, pkg))
            postinst += servicecreate_postinst_service

        d.setVar('pkg_postinst_%s' % pkg, postinst)
        bb.debug(1, 'pkg_postinst_%s = %s' % (pkg, d.getVar('pkg_postinst_%s' % pkg, True)))
}
