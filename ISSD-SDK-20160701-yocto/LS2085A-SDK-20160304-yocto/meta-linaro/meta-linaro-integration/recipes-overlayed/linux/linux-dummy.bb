SUMMARY = "Dummy Linux kernel"
DESCRIPTION = "Dummy Linux kernel, to be selected as the preferred \
provider for virtual/kernel to satisfy dependencies for situations \
where you wish to build the kernel externally from the build system."
SECTION = "kernel"

LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://${WORKDIR}/COPYING.GPL;md5=751419260aa954499f7abaabaa882bbe"

PROVIDES += "virtual/kernel"

PACKAGES_DYNAMIC += "^kernel-module-.*"
PACKAGES_DYNAMIC += "^kernel-image-.*"
PACKAGES_DYNAMIC += "^kernel-firmware-.*"

PACKAGES += "kernel-modules"
FILES_kernel-modules = ""
ALLOW_EMPTY_kernel-modules = "1"
DESCRIPTION_kernel-modules = "Kernel modules meta package"

# Set a fake kernel version to make various checks happy
KERNEL_VERSION = "3.18"

#COMPATIBLE_MACHINE = "your_machine"

PR = "r1"

SRC_URI = "file://COPYING.GPL"
S = "${WORKDIR}"

do_configure() {
	:
}

do_compile () {
	:
}

do_shared_workdir () {
	:
}

do_install() {
	# Stash fake data to get around the check in rootfs.py
	install -d ${D}${datadir}/kernel-depmod/
	echo "${KERNEL_VERSION}" > ${D}${datadir}/kernel-depmod/kernel-abiversion
	touch ${D}${datadir}/kernel-depmod/System.map-${KERNEL_VERSION}
}

do_bundle_initramfs() {
	:
}

do_deploy() {
	:
}

# Only stage the files we need for depmod, not the modules/firmware
sysroot_stage_all () {
	sysroot_stage_dir ${D}${datadir}/kernel-depmod ${SYSROOT_DESTDIR}${datadir}/kernel-depmod
}

emit_depmod_pkgdata() {
	# Stash data for depmod
	install -d ${PKGDESTWORK}/kernel-depmod/
	echo "${KERNEL_VERSION}" > ${PKGDESTWORK}/kernel-depmod/kernel-abiversion
	touch ${PKGDESTWORK}/kernel-depmod/System.map-${KERNEL_VERSION}
}

PACKAGEFUNCS += "emit_depmod_pkgdata"
 
addtask bundle_initramfs after do_install before do_deploy
addtask deploy after do_install
addtask shared_workdir after do_compile before do_install
