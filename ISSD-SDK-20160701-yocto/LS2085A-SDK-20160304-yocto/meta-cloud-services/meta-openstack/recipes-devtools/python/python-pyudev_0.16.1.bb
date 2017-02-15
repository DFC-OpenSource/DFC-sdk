DESCRIPTION = "A libudev binding"
HOMEPAGE = "http://pyudev.readthedocs.org/"
SECTION = "devel/python"
LICENSE = "LGPL-2.1"
LIC_FILES_CHKSUM = "file://COPYING;md5=a6f89e2100d9b6cdffcea4f398e37343"

PR = "r0"
SRCNAME = "pyudev"

SRC_URI = "https://pypi.python.org/packages/source/p/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "4034de584b6d9efcbfc590a047c63285"
SRC_URI[sha256sum] = "765d1c14bd9bd031f64e2612225621984cb2bbb8cbc0c03538bcc4c735ff1c95"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools 
