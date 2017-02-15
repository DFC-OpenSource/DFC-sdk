require cryptodev_${PV}.inc

SUMMARY = "A /dev/crypto device driver header file"

do_compile() {
	:
}

# Just install cryptodev.h which is the only header file needed to be exported
do_install() {
	install -D ${S}/crypto/cryptodev.h ${D}${includedir}/crypto/cryptodev.h
}

ALLOW_EMPTY_${PN} = "1"
BBCLASSEXTEND = "native nativesdk"
