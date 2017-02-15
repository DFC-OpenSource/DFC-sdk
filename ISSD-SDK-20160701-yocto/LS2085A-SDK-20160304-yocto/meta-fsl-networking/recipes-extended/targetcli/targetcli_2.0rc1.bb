SUMMARY = "Open Fabrics Enterprise Distribution (OFED)"
HOMEPAGE = "http://datera.org"
SECTION = "console/utils"
LICENSE = "BSD-3-Clause"
LIC_FILES_CHKSUM = "file://${WORKDIR}/targetcli-2.0rc1/COPYING;md5=73f1eb20517c55bf9493b7dd6e480788"

SRC_URI = "file://targetcli_2.0rc1.orig.tar.gz\
	  file://fabric.tar\
	  file://iser_patch.patch\
	  file://targetcli_logdir_changed_patch.patch"


SRC_URI[md5sum] = "5e5c6c4cb526b01ce3eacaec80d912ac"
SRC_URI[sha256sum] = "4b96c21fc759aa39092614c55575e3a478a5c8e7490e3cb5cfc1fde86f921ba1"

RDEPENDS_${PN} += "python"


do_patch() {

	echo "Patching targetcli for iser support and many more new updates"

	cd ${WORKDIR}/targetcli-2.0rc1
	patch -p1 < ${WORKDIR}/iser_patch.patch
	patch -p1 < ${WORKDIR}/targetcli_logdir_changed_patch.patch
	cd -
}

do_install() {

	echo "Installing targetcli 2.0rc1 "
	mkdir -p ${D}/etc
	mkdir -p ${D}/var
	cp -r ${WORKDIR}/targetcli-2.0rc1/  ${D}/etc/
	cp -r ${WORKDIR}/target/  ${D}/var/
}

