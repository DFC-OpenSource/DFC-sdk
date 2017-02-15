inherit hosts openstackchef

ROOTFS_POSTPROCESS_COMMAND += "openstack_configure_hosts ; "

openstack_configure_hosts() {
    bbnote "openstack: identifying hosts"

    echo "${CONTROLLER_IP} ${CONTROLLER_HOST}" >> ${IMAGE_ROOTFS}/etc/hosts
    echo "${COMPUTE_IP} ${COMPUTE_HOST}" >> ${IMAGE_ROOTFS}/etc/hosts
    echo "${MY_HOST}" > ${IMAGE_ROOTFS}/etc/hostname
}

