FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://dhclient-exit-hooks \
           "

do_install_append () {
	install -m 0644 ${WORKDIR}/dhclient-exit-hooks ${D}${sysconfdir}/dhcp/dhclient-exit-hooks
	sed 's%/etc/dhclient-exit-hooks%/etc/dhcp/dhclient-exit-hooks%g' -i ${D}${base_sbindir}/dhclient-script

	sed 's%request .*%\noption classless-static-routes code 121 = array of unsigned integer 8;\n\n&%g' -i ${D}${sysconfdir}/dhcp/dhclient.conf
	sed 's%netbios-name-servers,.*netbios-scope;%netbios-name-servers, netbios-scope, classless-static-routes;\n%g' -i ${D}${sysconfdir}/dhcp/dhclient.conf

}

FILES_${PN}-client += " \
	${sysconfdir}/dhcp/dhclient-exit-hooks \
	"
