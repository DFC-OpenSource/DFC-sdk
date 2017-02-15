# Specify the preferred version of toolchain

GDBVERSION = "7.8.2+fsl"

PREFERRED_VERSION_gdb = "${GDBVERSION}"
PREFERRED_VERSION_gdb-cross-${TARGET_ARCH} = "${GDBVERSION}"
PREFERRED_VERSION_gdb-cross-canadian-${TRANSLATED_TARGET_ARCH} = "${GDBVERSION}"
