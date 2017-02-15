MONITOR_STAGING_DIR="${STAGING_DIR}/monitor"
MONITOR_STAGING_SERVICES_DIR="${MONITOR_STAGING_DIR}/services"
MONITOR_STAGING_CHECKS_DIR="${MONITOR_STAGING_DIR}/checks"
MONITOR_CONFIG_DIR="${IMAGE_ROOTFS}/etc/monitor"
MONITOR_CONFIG_SERVICES_DIR="${MONITOR_CONFIG_DIR}/services"
MONITOR_CONFIG_CHECKS_DIR="${MONITOR_CONFIG_DIR}/checks"

addtask monitor_install before do_package after do_install
addtask monitor_clean before do_clean


def copy_check_files(d, check_var_name, src, dest):
    import shutil

    mon_checks = bb.data.getVar(check_var_name, d, 1)
    if mon_checks != None:
        if not os.path.exists(dest):
            os.makedirs(dest)
        for check in mon_checks.split():
            if os.path.exists(src + "/" + check):
                shutil.copy(src + "/" + check, dest)
                os.chmod(dest + "/" + check, 0755)

python do_monitor_install() {
    import shutil

    if base_contains('OPENSTACK_EXTRA_FEATURES', 'monitoring', "0", "1", d) == "1":
        bb.debug(1, 'OpenStack monitoring feature is disabled, skipping do_monitor_install')
        return

    mon_dir = bb.data.getVar('MONITOR_STAGING_DIR', d, 1)
    mon_services_dir = bb.data.getVar('MONITOR_STAGING_SERVICES_DIR', d, 1)
    mon_checks_dir = bb.data.getVar('MONITOR_STAGING_CHECKS_DIR', d, 1)
    if not os.path.exists(mon_dir):
        os.makedirs(mon_dir)
    if not os.path.exists(mon_services_dir):
        os.makedirs(mon_services_dir)
    if not os.path.exists(mon_checks_dir):
        os.makedirs(mon_checks_dir)
    workdir = d.getVar('WORKDIR', True)

    # Process monitor SERVICE catagory
    mon_service_pkgs = bb.data.getVar('MONITOR_SERVICE_PACKAGES', d, 1)
    if mon_service_pkgs != None:
        for pkg in mon_service_pkgs.split():
            f_name = os.path.join(mon_services_dir, pkg + '.service')
            if os.path.exists(f_name):
                os.remove(f_name)
            data = bb.data.getVar('MONITOR_SERVICE_' + pkg, d, 1)
            if data != None:
                f = open(f_name, 'w')
                f.write(d.getVar('MONITOR_SERVICE_' + pkg))

    # Process monior CHECKS catagory
    packages = (d.getVar('PACKAGES', True) or "").split()
    if len(packages) >= 1 and os.path.exists(mon_checks_dir):
        for pkg in packages:
            copy_check_files(d, 'MONITOR_CHECKS_' + pkg, workdir, mon_checks_dir + '/' + pkg)
}

python do_monitor_clean() {
    import shutil

    mon_dir = bb.data.getVar('MONITOR_STAGING_DIR', d, 1)
    mon_services_dir = bb.data.getVar('MONITOR_STAGING_SERVICES_DIR', d, 1)
    mon_checks_dir = bb.data.getVar('MONITOR_STAGING_CHECKS_DIR', d, 1)
    if not os.path.exists(mon_dir):
        return

    # Clean up monitor SERVICE catagory
    mon_service_pkgs = bb.data.getVar('MONITOR_SERVICE_PACKAGES', d, 1)
    if mon_service_pkgs != None and os.path.exists(mon_services_dir):
        for pkg in mon_service_pkgs.split():
            f_name = os.path.join(mon_services_dir, pkg + '.service')
            if os.path.exists(f_name):
                os.remove(f_name)

    # Process monior CHECKS catagory
    packages = (d.getVar('PACKAGES', True) or "").split()
    if len(packages) >= 1 and os.path.exists(mon_checks_dir):
        for pkg in packages:
            if bb.data.getVar('MONITOR_CHECKS_' + pkg, d, 1) != None:
                shutil.rmtree(mon_checks_dir + "/" + pkg, True)
}

monitor_rootfs_postprocess() {
    if ${@base_contains('OPENSTACK_EXTRA_FEATURES', 'monitoring', "false", "true", d)}; then
        echo "OpenStack monitoring feature is disabled, skipping monitor_rootfs_postprocess"
        exit
    fi

    mkdir -p ${MONITOR_CONFIG_DIR} > /dev/null 2>&1
    mkdir -p ${MONITOR_CONFIG_SERVICES_DIR} > /dev/null 2>&1
    mkdir -p ${MONITOR_CONFIG_CHECKS_DIR} > /dev/null 2>&1

    # Process monitor SERVICE catagory
    files_list=`find ${MONITOR_STAGING_SERVICES_DIR} -type f`
    for f in ${files_list}; do
        cat $f >> ${MONITOR_CONFIG_SERVICES_DIR}/service_list.cfg
        echo >> ${MONITOR_CONFIG_SERVICES_DIR}/service_list.cfg
    done

    # Process monior CHECKS catagory
    cp -rf ${MONITOR_STAGING_CHECKS_DIR}/* ${MONITOR_CONFIG_CHECKS_DIR}
}

ROOTFS_POSTPROCESS_COMMAND += "monitor_rootfs_postprocess ; "
