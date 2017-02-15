#
# Copyright (C) 2013 Wind River Systems, Inc.
#

SUMMARY = "Glusterfs distributed filesystem"
DESCRIPTION = "\
  GlusterFS is an open source, distributed file system capable of scaling \
  to several petabytes (actually, 72 brontobytes!) and handling thousands \
  of clients.  GlusterFS clusters together storage building blocks over \
  Infiniband RDMA or TCP/IP interconnect, aggregating disk and memory \
  resources and managing data in a single global namespace."
HOMEPAGE = "http://www.gluster.org/"
SECTION = "console/network"

SRC_URI += "file://glusterd-ovp \
            file://xlator-host-contamination-3.4.patch \
            file://glusterfs-disable-default-startup-scripts.patch \
            file://0001-Fix-for-mount.glusterfs-not-accepting-version-argume.patch \
           "

LICENSE = "(LGPLv3+ | GPLv2) & GPLv3+ & LGPLv3+ & GPLv2+ & LGPLv2+ & LGPLv2.1+ & Apache-2.0"
LIC_FILES_CHKSUM = "file://COPYING-GPLV2;md5=b234ee4d69f5fce4486a80fdaf4a4263 \
                    file://COPYING-LGPLV3;md5=e6a600fd5e1d9cbde2d983680233ad02 \
                    file://contrib/fuse-util/COPYING;md5=94d55d512a9ba36caa9b7df079bae19f"

inherit autotools update-rc.d pkgconfig

DEPENDS += "bison-native flex-native fuse libaio libxml2 ncurses \
            openssl python readline zlib"

EXTRA_OECONF = "ac_cv_file__etc_debian_version=no \
                ac_cv_file__etc_SuSE_release=no \
                ac_cv_file__etc_redhat_release=no \
                --with-mountutildir=${sbindir} \
                --disable-fusermount \
                PYTHON=${bindir}/python"

PACKAGECONFIG ??= "georeplication"
PACKAGECONFIG[bd-xlator] = "--enable-bd-xlator,--disable-bd-xlator,lvm2,"
PACKAGECONFIG[debug] = "--enable-debug,--disable-debug,,"
PACKAGECONFIG[georeplication] = "--enable-georeplication,--disable-georeplication,,rsync"
PACKAGECONFIG[libibverbs] = "--enable-ibverbs,--disable-ibverbs,libibverbs librdmacm,"
PACKAGECONFIG[ocf] = "--with-ocf,--without-ocf,,"
PACKAGECONFIG[systemtap] = "--enable-systemtap,--disable-systemtap,systemtap,"
PACKAGECONFIG[valgrind] = "--with-valgrind,--without-valgrind,valgrind,"

do_install_append() {
        install -d ${D}${sysconfdir}/init.d
        install -m 0755 ${WORKDIR}/glusterd-ovp \
          ${D}${sysconfdir}/init.d/glusterd

        # Mount looks for mount.* plug-ins in /sbin; fix it with a symlink.
        mkdir -p ${D}${base_sbindir}
        (cd ${D}${base_sbindir}; ln -s ..${sbindir}/mount.glusterfs .)

        # These are plug-ins, so they don't need libtool configs.
        find ${D}${libdir}/glusterfs/${PV} -name '*.la' -exec rm -f '{}' ';'

        # The RPM spec file creates these directories.
        install -d ${D}${sysconfdir}/default/volatiles
        cat > ${D}${sysconfdir}/default/volatiles/99_glusterfs << EOF
d root root 0755 ${localstatedir}/log/glusterfs none
d root root 0755 ${localstatedir}/run/gluster none
EOF
}

INITSCRIPT_PACKAGES = "glusterfs-server"
INITSCRIPT_NAME = "glusterd"
INITSCRIPT_PARAMS = "start 20 5 3 2 . stop 80 0 1 6 ."

# Allow plug-in symlinks.
INSANE_SKIP_${PN} += "dev-so"
INSANE_SKIP_${PN}-rdma += "dev-so"
INSANE_SKIP_${PN}-fuse += "dev-so"
INSANE_SKIP_${PN}-server += "dev-so"

PACKAGES += "${PN}-rdma ${PN}-geo-replication ${PN}-fuse ${PN}-server \
             ${PN}-api ${PN}-ocf"

FILES_${PN}-dbg += "${libdir}/glusterfs/${PV}/*/.debug \
                    ${libdir}/glusterfs/${PV}/*/*/.debug \
                    ${libdir}/glusterfs/${PV}/*/*/*/.debug \
                    ${libexecdir}/glusterfs/.debug"

FILES_${PN} = "${libdir}/glusterfs/${PV}/auth \
               ${libdir}/glusterfs/${PV}/rpc-transport/socket* \
               ${libdir}/glusterfs/${PV}/xlator/cluster \
               ${libdir}/glusterfs/${PV}/xlator/debug \
               ${libdir}/glusterfs/${PV}/xlator/encryption \
               ${libdir}/glusterfs/${PV}/xlator/features \
               ${libdir}/glusterfs/${PV}/xlator/performance \
               ${libdir}/glusterfs/${PV}/xlator/protocol/client* \
               ${libdir}/glusterfs/${PV}/xlator/system \
               ${libdir}/glusterfs/${PV}/xlator/testing \
               ${libdir}/libglusterfs.so.* \
               ${libdir}/libgfrpc.so.* \
               ${libdir}/libgfxdr.so.* \
               ${localstatedir} \
               ${sysconfdir}/default/volatiles \
               ${sbindir}/glusterfs \
               ${sbindir}/glusterfsd"

FILES_${PN}-rdma = "${libdir}/glusterfs/${PV}/rpc-transport/rdma*"

FILES_${PN}-geo-replication = "${libexecdir}/glusterfs/gsyncd \
                               ${libexecdir}/glusterfs/python/syncdaemon"

FILES_${PN}-fuse = "${bindir}/fusermount-glusterfs \
                    ${libdir}/glusterfs/${PV}/xlator/mount \
                    ${sbindir}/mount.glusterfs \
                    ${base_sbindir}/mount.glusterfs"

FILES_${PN}-server = "${libdir}/glusterfs/${PV}/xlator/mgmt \
                      ${libdir}/glusterfs/${PV}/xlator/nfs \
                      ${libdir}/glusterfs/${PV}/xlator/protocol/server* \
                      ${libdir}/glusterfs/${PV}/xlator/storage \
                      ${sbindir}/gluster \
                      ${sbindir}/glusterd \
                      ${sysconfdir}/glusterfs/glusterd.vol \
                      ${sysconfdir}/glusterfs/glusterfs-logrotate \
                      ${sysconfdir}/init.d/glusterd"

# Note: Debian package renaming produces libgfapi[0-9]+-*.*.rpm.
FILES_${PN}-api = "${libdir}/libgfapi.so.*"

FILES_${PN}-ocf = "${prefix}/lib/ocf"

pkg_postinst_${PN}() {
        if [ -z "$D" ] && [ -e /etc/init.d/populate-volatile.sh ]; then
                /etc/init.d/populate-volatile.sh update
        fi
}
