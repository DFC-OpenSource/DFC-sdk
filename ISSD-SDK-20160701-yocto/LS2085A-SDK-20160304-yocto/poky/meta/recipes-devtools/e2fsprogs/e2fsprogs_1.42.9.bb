require e2fsprogs.inc


SRC_URI += "file://acinclude.m4 \
            file://remove.ldconfig.call.patch \
            file://fix-icache.patch \
            file://quiet-debugfs.patch \
            file://0001-mke2fs-add-the-ability-to-copy-files-from-a-given-di.patch \
            file://0002-misc-create_inode.c-copy-files-recursively.patch \
            file://0003-misc-create_inode.c-create-special-file.patch \
            file://0004-misc-create_inode.c-create-symlink.patch \
            file://0005-misc-create_inode.c-copy-regular-file.patch \
            file://0006-misc-create_inode.c-create-directory.patch \
            file://0007-misc-create_inode.c-set-owner-mode-time-for-the-inod.patch \
            file://0008-mke2fs.c-add-an-option-d-root-directory.patch \
            file://0009-misc-create_inode.c-handle-hardlinks.patch \
            file://0010-debugfs-use-the-functions-in-misc-create_inode.c.patch \
            file://0011-mke2fs.8.in-update-the-manual-for-the-d-option.patch \
            file://0012-Fix-musl-build-failures.patch \
            file://0001-e2fsprogs-fix-cross-compilation-problem.patch \
            file://misc-mke2fs.c-return-error-when-failed-to-populate-fs.patch \
            file://cache_inode.patch \
            file://CVE-2015-0247.patch \
            file://0001-libext2fs-fix-potential-buffer-overflow-in-closefs.patch \
"

SRC_URI[md5sum] = "3f8e41e63b432ba114b33f58674563f7"
SRC_URI[sha256sum] = "2f92ac06e92fa00f2ada3ee67dad012d74d685537527ad1241d82f2d041f2802"

EXTRA_OECONF += "--libdir=${base_libdir} --sbindir=${base_sbindir} --enable-elf-shlibs --disable-libuuid --disable-uuidd --enable-verbose-makecmds"
EXTRA_OECONF_darwin = "--libdir=${base_libdir} --sbindir=${base_sbindir} --enable-bsd-shlibs"

do_configure_prepend () {
	cp ${WORKDIR}/acinclude.m4 ${S}/
}

do_install () {
	oe_runmake 'DESTDIR=${D}' install
	oe_runmake 'DESTDIR=${D}' install-libs
	# We use blkid from util-linux now so remove from here
	rm -f ${D}${base_libdir}/libblkid*
	rm -rf ${D}${includedir}/blkid
	rm -f ${D}${base_libdir}/pkgconfig/blkid.pc
	rm -f ${D}${base_sbindir}/blkid
	rm -f ${D}${base_sbindir}/fsck
	rm -f ${D}${base_sbindir}/findfs

	# e2initrd_helper and the pkgconfig files belong in libdir
	if [ ! ${D}${libdir} -ef ${D}${base_libdir} ]; then
		install -d ${D}${libdir}
		mv ${D}${base_libdir}/e2initrd_helper ${D}${libdir}
		mv ${D}${base_libdir}/pkgconfig ${D}${libdir}
	fi

	oe_multilib_header ext2fs/ext2_types.h
	install -d ${D}${base_bindir}
	mv ${D}${bindir}/chattr ${D}${base_bindir}/chattr.e2fsprogs

	install -v -m 755 ${S}/contrib/populate-extfs.sh ${D}${base_sbindir}/
}

do_install_append_class-target() {
	# Clean host path in compile_et, mk_cmds
	sed -i -e "s,ET_DIR=\"${S}/lib/et\",ET_DIR=\"${datadir}/et\",g" ${D}${bindir}/compile_et
	sed -i -e "s,SS_DIR=\"${S}/lib/ss\",SS_DIR=\"${datadir}/ss\",g" ${D}${bindir}/mk_cmds
}

RDEPENDS_e2fsprogs = "e2fsprogs-badblocks"
RRECOMMENDS_e2fsprogs = "e2fsprogs-mke2fs e2fsprogs-e2fsck"

PACKAGES =+ "e2fsprogs-e2fsck e2fsprogs-mke2fs e2fsprogs-tune2fs e2fsprogs-badblocks e2fsprogs-resize2fs"
PACKAGES =+ "libcomerr libss libe2p libext2fs"

FILES_e2fsprogs-resize2fs = "${base_sbindir}/resize2fs*"
FILES_e2fsprogs-e2fsck = "${base_sbindir}/e2fsck ${base_sbindir}/fsck.ext*"
FILES_e2fsprogs-mke2fs = "${base_sbindir}/mke2fs ${base_sbindir}/mkfs.ext* ${sysconfdir}/mke2fs.conf"
FILES_e2fsprogs-tune2fs = "${base_sbindir}/tune2fs ${base_sbindir}/e2label"
FILES_e2fsprogs-badblocks = "${base_sbindir}/badblocks"
FILES_libcomerr = "${base_libdir}/libcom_err.so.*"
FILES_libss = "${base_libdir}/libss.so.*"
FILES_libe2p = "${base_libdir}/libe2p.so.*"
FILES_libext2fs = "${libdir}/e2initrd_helper ${base_libdir}/libext2fs.so.*"
FILES_${PN}-dev += "${datadir}/*/*.awk ${datadir}/*/*.sed ${base_libdir}/*.so"

BBCLASSEXTEND = "native nativesdk"

inherit update-alternatives

ALTERNATIVE_${PN} = "chattr"
ALTERNATIVE_PRIORITY = "100"
ALTERNATIVE_LINK_NAME[chattr] = "${base_bindir}/chattr"
ALTERNATIVE_TARGET[chattr] = "${base_bindir}/chattr.e2fsprogs"
