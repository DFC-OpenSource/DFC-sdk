SUMMARY = "Graphical trace viewer for Ftrace"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=751419260aa954499f7abaabaa882bbe \
                    file://kernel-shark.c;beginline=6;endline=8;md5=2c22c965a649ddd7973d7913c5634a5e"
DEPENDS = "gtk+"
SRCREV = "79e08f8edb38c4c5098486caaa87ca90ba00f547"
PV = "2.3.2"

SRC_URI = "git://git.kernel.org/pub/scm/linux/kernel/git/rostedt/trace-cmd.git;branch=trace-cmd-stable-v2.3"

S = "${WORKDIR}/git"

do_install() {
    ${MAKE} NO_PYTHON=1 prefix=${prefix} plugin_dir=${libdir}/trace-cmd/plugins DESTDIR=${D} install_gui
    # remove files already shipped in trace-cmd package
    rm -f ${D}${bindir}/trace-cmd
    rm -rf ${D}${libdir}/trace-cmd
    rmdir ${D}${libdir}
}

RDEPENDS_${PN} = "trace-cmd"

FILESPATH = "${FILE_DIRNAME}/trace-cmd"
