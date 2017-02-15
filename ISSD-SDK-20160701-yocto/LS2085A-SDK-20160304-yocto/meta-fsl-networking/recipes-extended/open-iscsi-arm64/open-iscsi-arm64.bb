SUMMARY = "Open iscsi"
HOMEPAGE = "http://open-iscsi.org"
SECTION = "console/utils"
LICENSE = "BSD-3-Clause"
LIC_FILES_CHKSUM = "file://${WORKDIR}/usr/share/doc/open-iscsi/copyright;md5=665149c41ff41c49391f2d08c08fffbd"



SRC_URI = "file://open-iscsi-2.0.873+git0.3b4b4500.tgz"

SRC_URI[md5sum] = "593a42fb5377fec3d4bcd5d156bb5b0d"
SRCREV = "9102a5e3121e49a6f69fe5b0922ec142bc0b16767d9c52eb61af145f5bd366cb"


do_install() {

	echo "Installing open-iscsi-aarch64"
	cp -r ${WORKDIR}/usr ${D}
	cp -r ${WORKDIR}/var ${D}
	cp -r ${WORKDIR}/etc ${D}
	cp -r ${WORKDIR}/lib ${D}
}

INSANE_SKIP_${PN} += "installed-vs-shipped already-stripped"
