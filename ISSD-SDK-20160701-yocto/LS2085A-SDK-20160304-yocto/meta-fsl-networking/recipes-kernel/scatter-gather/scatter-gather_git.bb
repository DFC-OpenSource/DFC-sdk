SUMMARY = "Scatter-gather logic for multiple tables"
LICENSE = "GPLv2+"
LIC_FILES_CHKSUM = "file://COPYING;md5=e9605a22ea50467bd2bfe4cdd66e69ae"

do_configure[depends] += "virtual/kernel:do_shared_workdir"

inherit module

export INSTALL_MOD_DIR="kernel/scatter-gather"
export KERNEL_SRC="${STAGING_KERNEL_DIR}"

SRCBRANCH = "master"
SRC_URI = "git://sw-stash.freescale.net/scm/dsa/scatter-gather.git;branch=master;protocol=http"
SRCREV = "615a52e1e407acb658b9884a20e476d6041e3fbf"

S = "${WORKDIR}/git"
