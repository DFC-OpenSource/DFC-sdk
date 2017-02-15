require odp-common.inc

do_configure[noexec] = "1"
do_compile[noexec] = "1"

do_install () {
    install -d ${D}${includedir}/odp/flib/mc
    install -d ${D}${includedir}/odp/flib/qbman/include/drivers
    install -d ${D}${includedir}/odp/flib/linux
    install -d ${D}${includedir}/odp/nadk

    cp -rf ${S}/include/* ${D}${includedir}/odp/
    cp -rf ${S}/helper/include/odp/helper ${D}${includedir}/odp/odp/
    cp -rf ${S}/platform/linux-dpaa2/include/* ${D}${includedir}/odp/
    cp -rf ${S}/flib/linux/ls2bus ${D}${includedir}/odp/flib/linux/
    cp -rf ${S}/flib/mc/*.h ${D}${includedir}/odp/flib/mc/
    cp -rf ${S}/flib/qbman/include/drivers/*.h ${D}${includedir}/odp/flib/qbman/include/drivers

    rm -r ${D}${includedir}/odp/odp/api
}

ALLOW_EMPTY_${PN} = "1"
