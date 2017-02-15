DESCRIPTION = "OpenDataPlane (ODP) provides a data plane application programming \
environment that is easy to use, high performance, and portable between networking SoCs."
HOMEPAGE = "http://www.opendataplane.org/"
SECTION = "console/tools"
LICENSE = "BSD | GPLv2"
LIC_FILES_CHKSUM = "file://LICENSE;md5=4ccfa994aa96974cfcd39a59faee20a2"

SRC_URI = "git://sw-stash.freescale.net/scm/dpaa2/odp-nadk.git;branch=dpaa2;protocol=http \
           git://sw-stash.freescale.net/scm/dpaa2/qbman_userspace.git;protocol=http;name=qbman;destsuffix=git/flib/qbman \
           git://sw-stash.freescale.net/scm/sdk/flib.git;protocol=http;name=rta;destsuffix=git/flib/rta \
"
SRCREV = "f2bb202ad290a2bb20544db547460b5146673b07"
SRCREV_qbman = "3411ba5d92302a6195b97db61719f4b2b6df77de"
SRCREV_rta = "2a0e960263d55c16bc751b4eb30dd19c9391e3b7"

S = "${WORKDIR}/git"
