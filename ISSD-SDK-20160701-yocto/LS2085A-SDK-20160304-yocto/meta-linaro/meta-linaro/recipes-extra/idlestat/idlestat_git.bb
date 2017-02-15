DESCRIPTION = "tool to show how long a CPU or cluster enters idle state"
SUMMARY = "Idlestat is a tool which can show how long a CPU or cluster \
enters idle state. This infomation is obtained using traces from trace-cmd \
or ftrace tools."
LICENSE = "GPL-2.0"
LIC_FILES_CHKSUM = "file://Makefile;md5=1e2d28a88b081f97157089bb67d4249d"
SRCREV = "6de5e87ccf87beb0946c627c10554efb1480326f"
PV = "0.2+git${SRCPV}"

SRC_URI = "git://git.linaro.org/power/idlestat.git"

S = "${WORKDIR}/git"

do_install () {
    install -D -p -m0755 idlestat ${D}/${sbindir}/idlestat
}
