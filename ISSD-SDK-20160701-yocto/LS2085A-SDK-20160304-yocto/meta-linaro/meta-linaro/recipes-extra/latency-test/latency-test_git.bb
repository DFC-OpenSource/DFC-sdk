SUMMARY = "Latency test"
DESCRIPTION = "Test to measure the latency of the Linux kernel"
LICENSE = "BSD"
LIC_FILES_CHKSUM = "file://latency.c;endline=39;md5=0ac97d68f333880973b054365ea2acc5"
SRCREV = "e9671a414d832d37b16359b6a180c7c306199c55"

COMPATIBLE_HOST = "(i.86|x86_64|arm|aarch64).*-linux"

SRC_URI = "git://git.linaro.org/people/mike.holmes/latency-test.git"

S = "${WORKDIR}/git"

do_compile() {
    oe_runmake test
}

do_install() {
    install -D -p -m0755 latency ${D}${bindir}/latency
}
