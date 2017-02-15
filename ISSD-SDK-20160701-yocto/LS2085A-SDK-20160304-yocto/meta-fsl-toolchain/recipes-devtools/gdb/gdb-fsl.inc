PV = "7.8.2+fsl"

SRCBRANCH = "fsl_gdb_7_8-branch"
SRCREV = "b94b973ec54cfb82d71ad2dd363e3f617167f95f"
SRC_URI = "git://sw-stash.freescale.net/scm/sdk/gdb.git;branch=${SRCBRANCH};protocol=http;destsuffix=gdb-${PV}"

SRC_URI += "file://gdbserver-cflags-last.diff;striplevel=0 \
           "

EXTRA_OECONF += " --disable-binutils --disable-gas --disable-gold --disable-gprof --disable-ld "
