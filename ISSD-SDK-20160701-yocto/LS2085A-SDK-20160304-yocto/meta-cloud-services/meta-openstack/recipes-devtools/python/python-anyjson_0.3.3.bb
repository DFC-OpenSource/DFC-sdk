DESCRIPTION = "Loads the best available JSON implementation available in a common interface"
HOMEPAGE = "https://bitbucket.org/runeh/anyjson"
SECTION = "devel/python"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=989aa97e73c912a83a3c873fa11deb08"

PR = "r0"
SRCNAME = "anyjson"

SRC_URI = "http://pypi.python.org/packages/source/a/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"
S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

SRC_URI[md5sum] = "2ea28d6ec311aeeebaf993cb3008b27c"
SRC_URI[sha256sum] = "37812d863c9ad3e35c0734c42e0bf0320ce8c3bed82cd20ad54cb34d158157ba"

RDEPENDS_${PN} = "python-simplejson"
