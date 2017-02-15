require openvswitch.inc

RRECOMMENDS_${PN} += "kernel-module-openvswitch"

RDEPENDS_${PN}-ptest += "\
	python-logging python-syslog python-argparse python-io \
	python-fcntl python-shell python-lang python-xml python-math \
	python-datetime python-netclient python sed \
	"

SRC_URI += "\
	http://openvswitch.org/releases/openvswitch-${PV}.tar.gz \
	file://openvswitch-add-more-target-python-substitutions.patch \
	file://openvswitch-add-ptest.patch \
	file://run-ptest \
	"

SRC_URI[md5sum] = "d3c8a69df3d1b1a9eaef5a896576fd2a"
SRC_URI[sha256sum] = "43a2562fe5e8e48e997bfdb04691ffaaaefe73069b5699654538bf2f16ebfb1a"

LIC_FILES_CHKSUM = "file://COPYING;md5=e03b0d9c4115c44518594e5618e653f8"

inherit ptest

EXTRA_OEMAKE += "TEST_DEST=${D}${PTEST_PATH} TEST_ROOT=${PTEST_PATH}"

do_install_ptest() {
	oe_runmake test-install
}
