do_install_append() {
    if [ ! -f "${WORKDIR}/nova.conf" ]; then
        return
    fi

    TEMPLATE_CONF_DIR=${S}${sysconfdir}/${SRCNAME}
    NOVA_CONF_DIR=${D}/${sysconfdir}/nova

    sed -e "s:^virt_type.*=.*$:virt_type = qemu:" \
        -i ${NOVA_CONF_DIR}/nova.conf
}
