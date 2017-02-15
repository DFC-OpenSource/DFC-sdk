DESCRIPTION = "JQuery-Migrate JavaScript library packaged for setuptools"
HOMEPAGE = "https://pypi.python.org/pypi/XStatic-JQuery-Migrate"
SECTION = "devel/python"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://PKG-INFO;md5=2bbd78dd61f7b4f2fb6b3e31e4d7a26b"

PR = "r0"

SRCNAME = "XStatic-JQuery-Migrate"
SRC_URI = "http://pypi.python.org/packages/source/X/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "2dd39f9d6351aeaf129b33d4134ac6a7"
SRC_URI[sha256sum] = "e2959b3df49afdddb00d36b74cca727a91b994b9f4edb993d7264731a750900e"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

# DEPENDS_default: python-pip

DEPENDS += " \
        python-pip \
        "

# RDEPENDS_default: 
RDEPENDS_${PN} += " \
        "
