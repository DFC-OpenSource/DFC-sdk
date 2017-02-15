DESCRIPTION = "A Python re-implementation of the Rails routes system."
HOMEPAGE = "http://routes.groovie.org"
SECTION = "devel/python"
LICENSE = "BSD"
LIC_FILES_CHKSUM = "file://LICENSE;md5=aa6285dc7b7c93b3905bb2757f33eb62"

PR = "r0"
SRCNAME = "Routes"

SRC_URI = "http://pypi.python.org/packages/source/R/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "9740ff424ff6b841632c784a38fb2be3"
SRC_URI[sha256sum] = "eacc0dfb7c883374e698cebaa01a740d8c78d364b6e7f3df0312de042f77aa36"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

RDEPENDS_${PN} += "python-repoze.lru"
