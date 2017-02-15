FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI_append = " file://kbuild-include-config.mk-when-auto.conf-is-not-older.patch"
