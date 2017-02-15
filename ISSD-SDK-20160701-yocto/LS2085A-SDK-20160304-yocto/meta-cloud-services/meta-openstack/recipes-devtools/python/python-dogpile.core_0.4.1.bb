DESCRIPTION = "Python Dogpile Core: Dogpile is basically the locking code extracted from the Beaker package, for simple and generic usage."
HOMEPAGE = "https://pypi.python.org/pypi/dogpile.core"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=0401fd56584d7b3d4be91690672ec433"

PR = "r0"
SRCNAME = "dogpile.core"

SRC_URI = "https://pypi.python.org/packages/source/d/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "01cb19f52bba3e95c9b560f39341f045"
SRC_URI[sha256sum] = "be652fb11a8eaf66f7e5c94d418d2eaa60a2fe81dae500f3743a863cc9dbed76"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools


