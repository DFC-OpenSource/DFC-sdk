DESCRIPTION = "OpenStack test framework and test fixtures. \
The oslotest package can be cross-tested against its consuming projects to ensure \
that no changes to the library break the tests in those other projects."
HOMEPAGE = "https://pypi.python.org/pypi/oslotest"
SECTION = "devel/python"
LICENSE = "BSD"
LIC_FILES_CHKSUM = "file://LICENSE;md5=34400b68072d710fecd0a2940a0d1658"

PR = "r0"
SRCNAME = "oslotest"

SRC_URI = "https://pypi.python.org/packages/source/o/${SRCNAME}/${SRCNAME}-${PV}.tar.gz \
"

SRC_URI[md5sum] = "a573e1943869f0841f487cf94a121321"
SRC_URI[sha256sum] = "4226c044198792da7e14ca76895656e8d6e4b256eab184a6c81477b373e059a6"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

DEPENDS += " \
    python-pbr \
"

RDEPENDS_${PN} = "python-fixtures \
                  python-subunit \
                  python-testrepository \
                  python-testscenarios \
                  python-testtools \
                  python-mock \
                  python-mox \
                  python-pbr \
                  bash \
"
