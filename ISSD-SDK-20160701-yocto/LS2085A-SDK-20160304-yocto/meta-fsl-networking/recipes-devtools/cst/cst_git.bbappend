FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

# TODO: fix license - this file is not a license
LIC_FILES_CHKSUM = "file://common/include/global.h;beginline=4;endline=25;md5=2b4ca1812291dbb799e19c529c7e2666"

SRC_URI = "git://sw-stash.freescale.net/scm/sdk/cst.git;branch=QorIQ_LS;protocol=http \
           file://0001-Fix-the-compile-error.patch \
"
SRCREV = "59318fb0f4cdc9de57cfb3e65fd649d9137a9387"
