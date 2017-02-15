DESCRIPTION = "Ganglia is a scalable distributed monitoring \
system for high-performance computing systems such as \
clusters and Grids."
HOMEPAGE = "http://ganglia.sourceforge.net/"
SECTION = "console/utils"
LICENSE = "BSD"
LIC_FILES_CHKSUM = "file://NEWS;md5=ff8c91481123c7d3be4e31fcac997747"
DEPENDS = "apr confuse pcre python rrdtool"

SRC_URI = "\
    ${SOURCEFORGE_MIRROR}/${BPN}/${BPN}-${PV}.tar.gz \
    file://gmetad-example.conf \
    file://gmetad.init \
    file://gmond-example.conf \
    file://gmond.init \
    "
SRC_URI[md5sum] = "05926bb18c22af508a3718a90b2e9a2c"
SRC_URI[sha256sum] = "89eae02e1a117040d60b3b561fe55f88d7f8cf41b94af1492969ef68e6797886"

EXTRA_OECONF += " \
                 --with-gmetad \
                 --disable-python \
                "

inherit pkgconfig autotools-brokensep pythonnative update-rc.d

# The ganglia autoconf setup doesn't include libmetrics in its
# AC_OUTPUT list -- it reconfigures libmetrics using its own rules.
# Unfortunately this means an OE autoreconf will not regenerate
# ltmain.sh (and others) in libmetrics and as such the build will
# fail.  We explicitly force regeneration of that directory.

do_configure_append() {
       (cd ${S} ; autoreconf -fvi )
       (cd ${S}/libmetrics ; autoreconf -fvi)
}

do_install_append() {
    install -d ${D}${sysconfdir}/init.d
    # gmetad expects the following directory and owned by user 'nobody'
    install -o nobody -d ${D}${localstatedir}/lib/${PN}/rrds
    # gmond and gmetad configurations
    install -m 0644 ${WORKDIR}/gmetad-example.conf ${D}${sysconfdir}/gmetad.conf
    install -m 0644 ${WORKDIR}/gmond-example.conf ${D}${sysconfdir}/gmond.conf
    # Init scripts
    install -m 0755 ${WORKDIR}/gmetad.init ${D}${sysconfdir}/init.d/gmetad
    install -m 0755 ${WORKDIR}/gmond.init ${D}${sysconfdir}/init.d/gmond
    # Fixup hard-coded paths
    sed -i -e 's!^PATH=.*!PATH=${base_sbindir}:${sbindir}:${base_bindir}:${bindir}!' ${D}${sysconfdir}/init.d/gmetad
    sed -i -e 's!^PATH=.*!PATH=${base_sbindir}:${sbindir}:${base_bindir}:${bindir}!' ${D}${sysconfdir}/init.d/gmond
    sed -i -e 's!/etc/!${sysconfdir}/!g' ${D}${sysconfdir}/init.d/gmetad
    sed -i -e 's!/etc/!${sysconfdir}/!g' ${D}${sysconfdir}/init.d/gmond
    sed -i -e 's!/etc/conf.d/!${sysconfdir}/conf.d/!g' ${D}${sysconfdir}/gmond.conf
}

PACKAGES =+ "gmetad"

RDEPENDS_${PN} = "gmetad"

BBCLASSEXTEND = "native"

FILES_gmetad = "\
    ${sbindir}/gmetad \
    ${sysconfdir}/init.d/gmetad \
"

INITSCRIPT_PACKAGES = "${PN} gmetad"
INITSCRIPT_NAME_ganglia = "gmond"
INITSCRIPT_NAME_gmetad = "gmetad"
INITSCRIPT_PARAMS = "defaults 66"
