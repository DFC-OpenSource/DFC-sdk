do_install_append() {
	sed -i '2i port=`grep "^server.port" /etc/lighttpd.conf`; if [ -z $port ]; then echo "server port not configured, not running lighttpd..."; exit 0; fi' ${D}/etc/init.d/lighttpd
}
