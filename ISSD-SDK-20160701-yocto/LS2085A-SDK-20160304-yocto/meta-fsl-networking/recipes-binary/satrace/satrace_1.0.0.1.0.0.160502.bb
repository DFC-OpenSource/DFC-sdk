SUMMARY = "Linux SA trace component"
LICENSE = "Freescale-EULA"
LIC_FILES_CHKSUM = "file://COPYING;md5=bf20d39b348e1b0ed964c91a97638bbb"
RDEPENDS_${PN}-dev += "satrace"

FILES_${PN}-dev = "${libdir}/linux.armv8.satrace/lib/libls.target.agent.so.1.0 \
  ${libdir}/linux.armv8.satrace/lib/libls.target.agent.so \
  ${libdir}/linux.armv8.satrace/lib/libsae.ls.tc.config.so.1.0 \
  ${libdir}/linux.armv8.satrace/lib/libsae.ls.tc.config.so \
  ${libdir}/linux.armv8.satrace/lib/libls.linux.satrace.lib.so.1.0 \
  ${libdir}/linux.armv8.satrace/lib/libls.linux.satrace.lib.so \
  ${libdir}/linux.armv8.satrace/lib/libsae.ls.tc2.config.so.1.0 \
  ${libdir}/linux.armv8.satrace/lib/libsae.ls.tc2.config.so \
  ${libdir}/linux.armv8.satrace/lib/libsae.ls.target.access.so.1.0 \
  ${libdir}/linux.armv8.satrace/lib/libsae.ls.target.access.so \
  ${libdir}/linux.armv8.satrace/lib/libsae.ls.common.so.1.0 \
  ${libdir}/linux.armv8.satrace/lib/libsae.ls.common.so \
  ${libdir}/linux.armv8.satrace/bin/ls.linux.satrace \
  ${libdir}/linux.armv8.satrace/config/LS1023APlatformConfig.xml \
  ${libdir}/linux.armv8.satrace/config/LS2045APlatformConfig.xml \
  ${libdir}/linux.armv8.satrace/config/LS2085APlatformConfig.xml \
  ${libdir}/linux.armv8.satrace/config/LS2080APlatformConfig.xml \
  ${libdir}/linux.armv8.satrace/config/LS1043APlatformConfig.xml \
  ${libdir}/linux.armv8.satrace/config/LS2040APlatformConfig.xml \
  ${libdir}/linux.armv8.debugprint/lib/libls.linux.stm.debugprint.so.1.0 \
  ${libdir}/linux.armv8.debugprint/lib/libls.linux.debugprint.so.1.0 \
  ${libdir}/linux.armv8.debugprint/bin/ls.target.server \
  ${libdir}/linux.armv8.debugprint/config/LS2085ADebugPrintConfig.xml"

INHIBIT_PACKAGE_DEBUG_SPLIT = "1"

SRCBRANCH = "satrace_gcc4.9"
SRC_URI = "git://sw-stash.freescale.net/scm/dsa/sdk_components.git;branch=${SRCBRANCH};protocol=http"
SRCREV="c5b03c44c431bc964d3a0a270a58ca1a26de3db3"

S = "${WORKDIR}/git"

do_install() {
        oe_runmake install DESTDIR=${D}/${libdir}
        rm ${D}/${libdir}/COPYING
        rm ${D}/${libdir}/README
}

ALLOW_EMPTY_${PN} = "1"
