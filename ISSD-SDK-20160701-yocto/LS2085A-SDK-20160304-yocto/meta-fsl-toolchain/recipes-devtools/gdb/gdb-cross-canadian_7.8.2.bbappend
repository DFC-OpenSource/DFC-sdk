require gdb-fsl.inc

datadir .= "/gdb-${TUNE_PKGARCH}${TARGET_VENDOR}-${TARGET_OS}"

FILES_${PN}-dbg += "${prefix}/${TARGET_ARCH}${TARGET_VENDOR}-${TARGET_OS}/bin/.debug"
