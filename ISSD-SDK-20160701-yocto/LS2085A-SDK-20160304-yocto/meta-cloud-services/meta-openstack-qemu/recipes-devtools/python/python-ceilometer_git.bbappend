do_install_append() {
    if [ ! -f "${WORKDIR}/ceilometer.conf" ]; then
        return
    fi

    TEMPLATE_CONF_DIR=${S}${sysconfdir}/${SRCNAME}
    CEILOMETER_CONF_DIR=${D}/${sysconfdir}/ceilometer

    sed -e "s:^libvirt_type.*=.*$:libvirt_type = qemu:" \
        -i ${CEILOMETER_CONF_DIR}/ceilometer.conf
}