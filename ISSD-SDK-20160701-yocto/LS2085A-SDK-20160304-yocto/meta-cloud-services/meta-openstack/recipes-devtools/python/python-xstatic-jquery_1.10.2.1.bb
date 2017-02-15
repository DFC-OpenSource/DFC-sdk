DESCRIPTION = "jQuery javascript library packaged for setuptools (easy_install) / pip."
HOMEPAGE = "http://jquery.com/"
SECTION = "devel/python"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://README.txt;md5=af1f21066b501c7d9265fab0d5556ece"

PR = "r0"

SRCNAME = "XStatic-jQuery"
SRC_URI = "http://pypi.python.org/packages/source/X/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "7a29efeab6961ef00ea2272c923f4503"
SRC_URI[sha256sum] = "83416a6bb86e8534858c4d1ddca45e881c87639da6f78000c28c3a193fe91305"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

# DEPENDS_default: python-pip

DEPENDS += " \
        python-pip \
        "

# RDEPENDS_default: 
RDEPENDS_${PN} += " \
        "
