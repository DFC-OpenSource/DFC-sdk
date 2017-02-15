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

SRC_URI[md5sum] = "9c4d1471a56718132e0157af1bfc9310"
SRC_URI[sha256sum] = "011052645cd4c7afee2732e87d45e589a0540ac7b7523027d3be2d7c7db7c899"

LIC_FILES_CHKSUM = "file://COPYING;md5=5973c953e3c8a767cf0808ff8a0bac1b"

inherit ptest

EXTRA_OEMAKE += "TEST_DEST=${D}${PTEST_PATH} TEST_ROOT=${PTEST_PATH}"

CFLAGS_append = " -fPIC"

do_install_ptest() {
	oe_runmake test-install
}
