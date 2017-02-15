DESCRIPTION = "testscenarios: a pyunit extension for dependency injection"
HOMEPAGE = "https://pypi.python.org/pypi/testscenarios"
SECTION = "devel/python"
LICENSE = "BSD"
LIC_FILES_CHKSUM = "file://BSD;md5=0805e4f024d089a52dca0671a65b8b66"

PR = "r0"

SRCNAME = "testscenarios"
SRC_URI = "https://pypi.python.org/packages/source/t/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "433cb8cd4d444b0deded3787240ee586"
SRC_URI[sha256sum] = "4feeee84f7fd8a6258fc00671e1521f80cb68d2fec1e2908b3ab52bcf396e198"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

RDEPENDS_${PN} += " python-testtools"
