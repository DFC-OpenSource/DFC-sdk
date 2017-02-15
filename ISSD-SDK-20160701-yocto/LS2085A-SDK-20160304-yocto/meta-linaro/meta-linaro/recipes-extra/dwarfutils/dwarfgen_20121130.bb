require dwarf.inc

DEPENDS = "libdwarf libdwarf-native"

LIC_FILES_CHKSUM = "file://GPL.txt;md5=751419260aa954499f7abaabaa882bbe"

do_install() {
    install -d ${D}${bindir} ${D}${mandir}/man1
    install -m 0755 ${BPN} ${D}${bindir}
    install -m 0644 ${BPN}.1 ${D}${mandir}/man1
}

PARALLEL_MAKE = ""
