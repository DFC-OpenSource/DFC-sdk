require nadk.inc

EXTRA_OEMAKE_append = ' NADK_SHARED=1'

do_install () {
    oe_runmake install-bins NADK_bin_dir=${D}/usr/nadk/nadk-dynamic/bin
}

FILES_${PN}-dbg += "/usr/nadk/nadk-dynamic/bin/.debug"
