FILESEXTRAPATHS_append := "${THISDIR}/${PN}"

RRECOMMENDS_${PN} += "kernel-module-veth \
	"
