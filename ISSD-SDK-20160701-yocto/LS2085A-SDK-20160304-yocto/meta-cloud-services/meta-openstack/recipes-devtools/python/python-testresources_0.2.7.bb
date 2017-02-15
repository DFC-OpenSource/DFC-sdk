DESCRIPTION = "Testresources, a pyunit extension for managing expensive test resources"
HOMEPAGE = "https://pypi.python.org/pypi/testtools/"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://README;md5=038679cd2cf27bb2acc70257bfee0f41"

PR = "r0"
SRCNAME = "testresources"

SRC_URI = "https://pypi.python.org/packages/source/t/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "db2e774be2a6f5754cbbf4c537f823d0"
SRC_URI[sha256sum] = "ad0a117383dd463827b199eaa92829b4d6a3147fbd97459820df53bae81d7231"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools
