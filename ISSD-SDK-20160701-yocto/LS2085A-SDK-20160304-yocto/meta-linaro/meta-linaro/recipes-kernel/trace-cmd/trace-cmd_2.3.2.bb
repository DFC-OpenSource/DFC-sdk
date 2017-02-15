SUMMARY = "User interface to Ftrace"
LICENSE = "GPLv2 & LGPLv2.1"
LIC_FILES_CHKSUM = "file://COPYING;md5=751419260aa954499f7abaabaa882bbe \
                    file://trace-cmd.c;beginline=6;endline=8;md5=2c22c965a649ddd7973d7913c5634a5e \
                    file://COPYING.LIB;md5=bbb461211a33b134d42ed5ee802b37ff \
                    file://trace-input.c;beginline=5;endine=8;md5=dafd8a1cade30b847a8686dd3628cea4"

DEPENDS = "swig-native"

SRCREV = "79e08f8edb38c4c5098486caaa87ca90ba00f547"
PV = "2.3.2"

SRC_URI = "git://git.kernel.org/pub/scm/linux/kernel/git/rostedt/trace-cmd.git;branch=trace-cmd-stable-v2.3"

S = "${WORKDIR}/git"

do_install() {
    ${MAKE} NO_PYTHON=1 prefix=${prefix} plugin_dir=${libdir}/trace-cmd/plugins DESTDIR=${D} install
}

FILES_${PN}-dbg += "${libdir}/trace-cmd/plugins/.debug/"
