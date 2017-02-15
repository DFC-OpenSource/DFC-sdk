SUMMARY = "a Linux System call fuzz teste."
DESCRIPTION = "Trinity, a Linux System call fuzz tester."
HOMEPAGE = "http://codemonkey.org.uk/projects/trinity/"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://${S}/COPYING;md5=96094d47cfbd2cc45eb46ce0fc423c04"

COMPATIBLE_HOST = "(x86_64|arm|aarch64).*-linux"

PV = "1.4+1.5pre"
SRCREV = "985a08744aeb8bccb0b50145375092e2a466a6da"
SRC_URI = "git://github.com/kernelslacker/trinity.git;protocol=https \
          "

S = "${WORKDIR}/git"

inherit useradd

USERADD_PACKAGES = "${PN}"
USERADD_PARAM_${PN} = "--system --create-home --shell /bin/sh ${PN} "

do_configure () {
    ./configure.sh
}

# workaround random build failures
do_compile () {
    ${MAKE}
}

do_install () {
    oe_runmake install DESTDIR=${D}/usr
    install -o ${PN} -d -m 0755 ${D}/${datadir}/${PN}
    install -o ${PN} -m 0755 ${S}/scripts/test-all-syscalls-parallel.sh     ${D}/${datadir}/${PN}
    install -o ${PN} -m 0755 ${S}/scripts/test-all-syscalls-sequentially.sh ${D}/${datadir}/${PN}
    install -o ${PN} -m 0755 ${S}/scripts/test-multi.sh                     ${D}/${datadir}/${PN}
    install -o ${PN} -m 0755 ${S}/scripts/test-vm.sh                        ${D}/${datadir}/${PN}
}

PACKAGES =+ "${PN}-example"

FILES_${PN} = "${bindir}/trinity"
FILES_${PN}-example = "${datadir}/${PN}"
