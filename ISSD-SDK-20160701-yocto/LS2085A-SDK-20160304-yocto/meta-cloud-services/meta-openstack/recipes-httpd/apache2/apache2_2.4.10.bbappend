do_install_append() {
    sed -i -e 's/#ServerName.*$/ServerName 127.0.0.1:80/' ${D}${sysconfdir}/apache2/httpd.conf

    # default layout for htdocsdir in 2.4.7 is different, create the following for
    # backward compatibility reasons

    mkdir -p ${D}${datadir}/${BPN}/default-site
    ln -sf ../htdocs ${D}${datadir}/${BPN}/default-site/htdocs
    
    sed -i '30i if [ ! -e /var/log/apache2 ]; then ln -sf /var/apache2/logs/ /var/log/apache2; fi' ${D}/etc/init.d/apache2
}

FILES_${PN} += "${datadir}/${BPN}/default-site"
