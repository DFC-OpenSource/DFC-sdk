require dwarf.inc

DEPENDS = "libdwarf libdwarf-native"

LIC_FILES_CHKSUM = "file://GPL.txt;md5=751419260aa954499f7abaabaa882bbe"

SRC_URI += "file://fix-dump.patch"

do_compile_prepend() {
	oe_runmake CC="${BUILD_CC}" \
		CFLAGS="${BUILD_CFLAGS} -I${S}/../libdwarf/ -I." \
		LDFLAGS="${BUILD_LDFLAGS} -ldwarf -lelf" NATIVE=1
	rm *.o dwarfdump
}

do_install() {
    install -d ${D}${bindir} ${D}${mandir}/man1
    install -m 0755 dwarfdump ${D}${bindir}
    install -m 0644 dwarfdump.1 ${D}${mandir}/man1
}

PARALLEL_MAKE = ""
