DESCRIPTION = "HP 3PAR HTTP REST Client"
HOMEPAGE = "https://pypi.python.org/pypi/hp3parclient/2.0.0"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://PKG-INFO;md5=497435a85c6b1376c82b18488e9bc907"

PR = "r0"
SRCNAME = "hp3parclient"

SRC_URI = "\
	https://pypi.python.org/packages/source/h/${SRCNAME}/${SRCNAME}-${PV}.tar.gz \
	file://fix_hp3parclient_memory_leak.patch \
	"

SRC_URI[md5sum] = "f4cc346281ae86c21b5f975cc3b4d759"
SRC_URI[sha256sum] = "83c0c00a5ba9fd5cecf6f32c6aea9d222e34abcb521548988b70ac8d062ec2f2"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools
