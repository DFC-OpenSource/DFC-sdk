FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

RDEPENDS_${PN}-tests += "bash"

SRC_URI_append = " file://0001-open-iscsi-add-aarch64-support.patch"

FILES_${PN} += "/run"
