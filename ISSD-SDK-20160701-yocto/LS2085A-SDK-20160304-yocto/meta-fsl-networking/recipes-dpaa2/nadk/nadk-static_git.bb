require nadk.inc

do_install () {
    oe_runmake install-bins NADK_bin_dir=${D}/usr/nadk/nadk-static/bin
}

FILES_${PN}-dbg += "/usr/nadk/nadk-static/bin/.debug"
