DESCRIPTION = "Datapath layout files"
SECTION = "dpaa2"
LICENSE = "Freescale-EULA"
LIC_FILES_CHKSUM = "file://Freescale-EULA;md5=f8756d9df61899a909c957a63a791835"

DEPENDS = "dtc-native"

S = "${WORKDIR}/git"

inherit deploy

SRC_URI = "git://sw-stash.freescale.net/scm/dpaa2/dpl-examples.git;branch=master;protocol=http"
SRCREV = "e3d836ba298c5136d5819e28e8184a3654a2f9db"

TP ?= "RDB"
TP_ls2080aqds = "QDS"
TP_ls2085aqds = "QDS"
TP_ls2085aissd = "ISSD"

do_patch() {
    M=`echo ${MACHINE} | sed -e 's,[b-z]*$,,'`
	if [ "${TP}" = "ISSD" ]
	then
  	mkdir -p ${S}/${M}/${TP}
	cp -r ${TOPDIR}/../meta-fsl-networking/recipes-dpaa2/dpl-examples/files/*.dts ${S}/${M}/${TP}
	fi
}

do_install () {
    M=`echo ${MACHINE} | sed -e 's,[b-z]*$,,'`
	cp -r ${S}/${M}/QDS/dpc* ${S}/${M}/${TP}/
    install -d ${D}/boot
    install -m 644 ${S}/${M}/${TP}/*.dtb ${D}/boot
}

do_deploy () {
	M=`echo ${MACHINE} | sed -e 's,[b-z]*$,,'`
    install -d ${DEPLOYDIR}/dpl-examples
    install -m 644 ${S}/${M}/${TP}/*.dtb ${DEPLOYDIR}/dpl-examples
}
addtask deploy before do_build after do_install

PACKAGES += "${PN}-image"
FILES_${PN}-image += "/boot"

COMPATIBLE_MACHINE = "(ls2085aqds|ls2085ardb|ls2080ardb|ls2080aqds|ls2085aissd)"
