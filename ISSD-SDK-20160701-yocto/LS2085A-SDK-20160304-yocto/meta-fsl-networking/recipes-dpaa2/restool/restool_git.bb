DESCRIPTION = "DPAA2 Resource Manager Tool"
SECTION = "dpaa2"
LICENSE = "Freescale-EULA"
LIC_FILES_CHKSUM = "file://Freescale-EULA;md5=395c11b7d81446eaa8f997521afe0ebb"

RDEPENDS_${PN} = "bash"

SRC_URI = "git://sw-stash.freescale.net/scm/dpaa2/restool.git;branch=master;protocol=http"
SRCREV = "02fc9b2e3f214ba269356a4165b0fd752dc73387"

S = "${WORKDIR}/git"

do_compile_prepend () {
    unset CFLAGS
}

do_install () {
    oe_runmake install DESTDIR=${D}
}
