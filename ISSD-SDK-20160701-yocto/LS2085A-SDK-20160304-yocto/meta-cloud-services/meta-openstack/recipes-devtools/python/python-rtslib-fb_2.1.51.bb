DESCRIPTION = "API for Linux kernel SCSI target (aka LIO)"
HOMEPAGE = "http://github.com/agrover/rtslib-fb"
SECTION = "devel/python"
LICENSE = "Apache-2"
LIC_FILES_CHKSUM = "file://PKG-INFO;md5=867c358d5dbac2602d0c0f850f731e62"


SRCNAME = "rtslib-fb"
SRC_URI = "http://pypi.python.org/packages/source/r/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "4ec7575eb90b7c5a91bbea2962f0e036"
SRC_URI[sha256sum] = "1902c581d6e04b7813f3cd1b11e2abc796205f646c39571cbbb809229fd5553e"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

# DEPENDS_default: python-pip

DEPENDS += " \
        python-pip \
        "

# RDEPENDS_default: 
RDEPENDS_${PN} += " \
        "
