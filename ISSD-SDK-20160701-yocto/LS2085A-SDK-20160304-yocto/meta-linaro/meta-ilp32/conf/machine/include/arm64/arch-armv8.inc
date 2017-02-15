DEFAULTTUNE ?= "aarch64"

ARMPKGARCH ?= "aarch64"

TUNEVALID[aarch64] = "Enable instructions for aarch64"
TUNEVALID[bigendian] = "Enable big-endian mode."
TUNECONFLICTS[aarch64] = ""
MACHINEOVERRIDES .= "${@bb.utils.contains("TUNE_FEATURES", "aarch64", ":aarch64", "" ,d)}"

TUNEVALID[ilp32] = "Enable instructions for aarch64 32 bit mode"
#TUNECONFLICTS[ilp32] = "aarch64"
ABIEXTENSION .= "${@bb.utils.contains("TUNE_FEATURES", "ilp32", "ilp32", "" ,d)}"
TUNE_CCARGS .= "${@bb.utils.contains("TUNE_FEATURES", "ilp32", " -mabi=ilp32", "", d)}"
MACHINEOVERRIDES .= "${@bb.utils.contains("TUNE_FEATURES", "ilp32", ":aarchilp32", "" ,d)}"

# Little Endian base configs
AVAILTUNES += "aarch64 aarch64_be aarch64_ilp32 aarch64_be_ilp32"
TUNE_FEATURES_tune-aarch64 ?= "aarch64"
TUNE_FEATURES_tune-aarch64_be ?= "${TUNE_FEATURES_tune-aarch64} bigendian"
TUNE_FEATURES_tune-aarch64_ilp32 ?= "aarch64 ilp32"
TUNE_FEATURES_tune-aarch64_be_ilp32 ?= "${TUNE_FEATURES_tune-aarch64} bigendian ilp32"

ARMPKGSFX_ENDIAN = "${@bb.utils.contains("TUNE_FEATURES", "bigendian", "_be", "", d)}"
ARMPKGSFX_ILP32 = "${@bb.utils.contains("TUNE_FEATURES", "ilp32", "_ilp32", "", d)}"

TUNE_ARCH = "aarch64${ARMPKGSFX_ENDIAN}"
TUNE_PKGARCH = "aarch64${ARMPKGSFX_ENDIAN}${ARMPKGSFX_ILP32}"

PACKAGE_EXTRA_ARCHS = "aarch64${ARMPKGSFX_ENDIAN}${ARMPKGSFX_ILP32}"
