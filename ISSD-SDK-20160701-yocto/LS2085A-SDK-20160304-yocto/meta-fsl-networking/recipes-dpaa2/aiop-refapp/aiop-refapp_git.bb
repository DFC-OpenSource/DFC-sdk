DESCRIPTION = "DPAA2 AIOP Packet Reflector Application"
SECTION = "dpaa2"
LICENSE = "BSD"
LIC_FILES_CHKSUM = "file://demos/classifier/src/apps.h;beginline=2;endline=24;md5=ee87f38a0b9280575e834a11851d9bd3"

BASEDEPENDS = ""

S = "${WORKDIR}/git/${BPN}"

SRC_URI = " \
  git://sw-stash.freescale.net/scm/dpaa2/aiop-refapp.git;branch=ls2_ear_devel;protocol=http;destsuffix=git/aiop-refapp \
  git://sw-stash.freescale.net/scm/dpaa2/aiopsl.git;branch=rel_0.8_rc1_fix;protocol=http;name=aiopsl;destsuffix=git/aiopsl \
"
SRCREV = "ec98866a7dff5133dcb944608cb4357540be4e7b"
SRCREV_aiopsl = "0aa8ad7ccad2c2ead8cdbd88ee5916651f402ab0"

do_configure[noexec] = "1"
do_compile[noexec] = "1"

do_install () {
    install -d ${D}/usr/aiop/bin
    install -d ${D}/usr/aiop/scripts
    install -d ${D}/usr/aiop/traffic_files
    install -m 755 ${S}/demos/images/aiop_*.elf ${D}/usr/aiop/bin
    install -m 755 ${S}/scripts/*.sh ${D}/usr/aiop/scripts
    install -m 644 ${S}/demos/traffic_files/classifier.pcap ${D}/usr/aiop/traffic_files
}

FILES_${PN} += "/usr/aiop/*"
INSANE_SKIP_${PN} += "arch"
INHIBIT_PACKAGE_STRIP = "1"
