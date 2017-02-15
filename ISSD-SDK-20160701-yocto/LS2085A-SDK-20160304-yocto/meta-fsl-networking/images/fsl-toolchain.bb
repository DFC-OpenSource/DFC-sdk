require recipes-core/meta/meta-toolchain.bb

TOOLCHAIN_NEED_CONFIGSITE_CACHE += "zlib"
TOOLCHAIN_TARGET_TASK += " \
	dtc-staticdev \
	libgomp \
	libgomp-dev \
	libgomp-staticdev \
	libstdc++-staticdev \
	${TCLIBC}-staticdev \
	"

TOOLCHAIN_HOST_TASK += " \
	nativesdk-cst \
	nativesdk-dtc \
	nativesdk-u-boot-mkimage \
"
