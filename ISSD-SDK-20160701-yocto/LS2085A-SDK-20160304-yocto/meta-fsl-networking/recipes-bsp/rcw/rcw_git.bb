DESCRIPTION = "Reset Control Words (RCW)"
SECTION = "rcw"
LICENSE = "BSD"

#NEED FIX
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/BSD;md5=3775480a712fc46a69647678acb234cb"

# this package is specific to the machine itself
INHIBIT_DEFAULT_DEPS = "1"
PACKAGE_ARCH = "${MACHINE_ARCH}"

inherit deploy

#SRC_URI = "git://sw-stash.freescale.net/scm/dpaa2/ls2-rcw.git;branch=master;protocol=http"
#SRCREV = "8afae160856e48e7db42830e9967696c582031ff"
SRC_URI = "file://fslu_nvme_ear6_rcw.tar.bz2;name=tarball"
SRCREV = "63d14f4435c347a7c6493a3c41191997"

S = "${WORKDIR}"

TP ?= "RDB"
TP_ls2080aqds = "QDS"
TP_ls2085aqds = "QDS"
TP_ls2085aissd = "ISSD"

do_install () {
    M=`echo ${MACHINE} | sed s/ls2085a//g | tr [a-z] [A-Z]`
    install -d ${D}/boot/rcw
    if [ "$M" = "ISSD" ]
    then
        mkdir -p ${S}/images/${M}
        cp -r ${S}/images/QDS/* ${S}/images/${M}/.
    fi

    cp -r ${S}/images/${M}/* ${D}/boot/rcw
}

do_deploy () {
	M=`echo ${MACHINE} | sed s/ls2085a//g | tr [a-z] [A-Z]`	
    install -d ${DEPLOYDIR}/rcw
    cp -r ${S}/images/${M}/* ${DEPLOYDIR}/rcw
}
addtask deploy after do_install

PACKAGES += "${PN}-image"
FILES_${PN}-image += "/boot"

COMPATIBLE_MACHINE = "(ls2085aqds|ls2085ardb|ls2080ardb|ls2080aqds|ls2085aissd)"
