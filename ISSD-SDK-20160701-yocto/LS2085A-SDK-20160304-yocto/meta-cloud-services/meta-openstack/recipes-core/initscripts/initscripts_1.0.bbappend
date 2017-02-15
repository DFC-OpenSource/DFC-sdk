do_install_append() {

	echo                  >> ${D}${sysconfdir}/init.d/functions
	echo init_is_upstart \(\) \{  >> ${D}${sysconfdir}/init.d/functions
	echo \ \ \ \ false >> ${D}${sysconfdir}/init.d/functions
	echo \}               >> ${D}${sysconfdir}/init.d/functions
	echo log_daemon_msg \(\) \{  >> ${D}${sysconfdir}/init.d/functions
	echo \ \ \ \ echo \$* >> ${D}${sysconfdir}/init.d/functions
	echo \}               >> ${D}${sysconfdir}/init.d/functions
}
