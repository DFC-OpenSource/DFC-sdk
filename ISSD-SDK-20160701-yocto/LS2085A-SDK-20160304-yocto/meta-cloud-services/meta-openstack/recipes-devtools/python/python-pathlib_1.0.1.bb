SUMMARY = "Object-oriented filesystem paths"
DESCRIPTION = "pathlib offers a set of classes to handle filesystem paths. \
It offers the following advantages over using string objects: \
- No more cumbersome use of os and os.path functions. \
- Embodies the semantics of different path types. \
- Well-defined semantics, eliminating any warts or ambiguities. \
"
HOMEPAGE = "https://pypi.python.org/pypi/pathlib"
SECTION = "devel/python"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE.txt;md5=3b6557c860f0fc867aeab30afd649753"


SRCNAME = "pathlib"
SRC_URI = "http://pypi.python.org/packages/source/p/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "5099ed48be9b1ee29b31c82819240537"
SRC_URI[sha256sum] = "6940718dfc3eff4258203ad5021090933e5c04707d5ca8cc9e73c94a7894ea9f"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

DEPENDS += " \
        python-pip \
"

RDEPENDS_${PN} += " \
"
