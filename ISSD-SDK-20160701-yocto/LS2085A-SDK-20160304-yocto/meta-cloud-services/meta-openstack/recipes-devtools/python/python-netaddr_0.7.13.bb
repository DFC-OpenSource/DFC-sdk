DESCRIPTION = "A library for network address representation and manipulation"
HOMEPAGE = "https://github.com/drkjam/netaddr"
SECTION = "devel/python"
LICENSE = "BSD"
LIC_FILES_CHKSUM = "file://LICENSE;md5=b277425f87f3b06d25af45d8b96f9682"

PR = "r0"
SRCNAME = "netaddr"

SRC_URI = "https://pypi.python.org/packages/source/n/${SRCNAME}/${SRCNAME}-${PV}.zip"

SRC_URI[md5sum] = "fcf004101890f40fe9980d6980c695ba"
SRC_URI[sha256sum] = "66b39922fabf219bb419231d370262191d2e38c93e96ca343c0c27a712f6cf67"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit distutils
