DESCRIPTION = "A repository of test results"
HOMEPAGE = "https://pypi.python.org/pypi/testrepository/"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://COPYING;md5=f19071a777e237c55ec3ab83284b31b8"

PR = "r0"
SRCNAME = "testrepository"

SRC_URI = "https://pypi.python.org/packages/source/t/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "f1f234caa753179ccfad8f7f764ddde4"
SRC_URI[sha256sum] = "ba15301c6ec6bf1b8e0dad10ac7313b11e17ceb8d28ec4a3625c9aaa766727fd"


S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

RDEPENDS_${PN} += "python-subunit \
                   python-extras \
"

CLEANBROKEN = "1"

