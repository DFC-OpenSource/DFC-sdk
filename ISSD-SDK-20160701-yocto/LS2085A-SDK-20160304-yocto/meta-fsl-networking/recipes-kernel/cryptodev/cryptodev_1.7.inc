HOMEPAGE = "http://cryptodev-linux.org/"

RCONFLICTS_${PN} = "ocf-linux"
RREPLACES_${PN} = "ocf-linux"

LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=b234ee4d69f5fce4486a80fdaf4a4263"

SRC_URI = "http://download.gna.org/cryptodev-linux/cryptodev-linux-${PV}.tar.gz"
SRC_URI[md5sum] = "0b63b3481cf2c90386b35f057481d36b"
SRC_URI[sha256sum] = "41880533b53de4d7b3f054e230f576988dafb8eed7bef5ebcf6422bb2e3a3b25"

# Upstream hotfixes and yocto specific patches
SRC_URI_append = " \
file://0001-fix-compilation-against-linux-3.19.patch \
file://0002-tests-Makefile-fix-arg-passing-to-CC-in-implicit-rul.patch \
file://0003-Disable-installing-header-file-provided-by-another-p.patch \
file://0004-Add-the-compile-and-install-rules-for-cryptodev-test.patch \
"

S = "${WORKDIR}/cryptodev-linux-${PV}"
