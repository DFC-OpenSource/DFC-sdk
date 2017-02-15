require odp.inc

do_install_append () {
    install -d ${D}/usr/odp/scripts
    mv ${D}${bindir} ${D}/usr/odp

    rm -fr ${D}${includedir}

    mv ${D}/usr/odp/bin/run_* ${D}/usr/odp/scripts
    install -m 755 ${S}/scripts/dpaa2/* ${D}/usr/odp/scripts
}

FILES_${PN} += "/usr/odp/bin/* /usr/odp/scripts/*"
FILES_${PN}-dbg += "/usr/odp/bin/.debug"
FILES_${PN}-staticdev += "${datadir}/opendataplane/*.la"
