SUMMARY = "An image containing the cirros cloud guest"
DESCRIPTION = "CirrOS a tiny cloud guest"
HOMEPAGE = "https://launchpad.net/cirros"

LICENSE="GPLv2"

SRC_URI = "http://download.cirros-cloud.net/${PV}/${PN}-${PV}-x86_64-disk.img"

SRC_URI[md5sum] = "64d7c1cd2b6f60c92c14662941cb7913"
SRC_URI[sha256sum] = "a2ca56aeded5a5bcaa6104fb14ec07b1ceb65222e2890bef8a89b8d2da729ad5"

LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/GPL-2.0;md5=801f80980d171dd6425610833a22dbe6"

INHIBIT_PACKAGE_STRIP="1"

do_install() {
	     install -d ${D}/${ROOT_HOME}/images
	     install -m 755 ${WORKDIR}/${PN}-${PV}-x86_64-disk.img ${D}/${ROOT_HOME}/images
}

PACKAGES = "cirros-guest-image"
FILES_cirros-guest-image = "${ROOT_HOME}/images/*"
