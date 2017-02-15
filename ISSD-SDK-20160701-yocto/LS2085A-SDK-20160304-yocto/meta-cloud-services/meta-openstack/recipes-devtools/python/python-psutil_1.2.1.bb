SUMMARY = "A cross-platform process and system utilities module for Python"
SECTION = "devel/python"
HOMEPAGE = "http://code.google.com/p/psutil"
LICENSE = "BSD"
LIC_FILES_CHKSUM = "file://LICENSE;md5=0f02e99f7f3c9a7fe8ecfc5d44c2be62"

SRCNAME = "psutil"

SRC_URI = "https://pypi.python.org/packages/source/p/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"
SRC_URI[md5sum] = "80c3b251389771ab472e554e6c729c36"
SRC_URI[sha256sum] = "508e4a44c8253a386a0f86d9c9bd4a1b4cbb2f94e88d49a19c1513653ca66c45"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

