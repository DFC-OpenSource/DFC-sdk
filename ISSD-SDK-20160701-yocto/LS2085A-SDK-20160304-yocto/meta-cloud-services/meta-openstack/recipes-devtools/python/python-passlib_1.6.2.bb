DESCRIPTION = "comprehensive password hashing framework supporting over 30 schemes"
HOMEPAGE = "http://passlib.googlecode.com"
SECTION = "devel/python"
LICENSE = "BSD"
LIC_FILES_CHKSUM = "file://LICENSE;md5=ec76a9db3f987418e132c0f0210e5ab1"

PR = "r0"
SRCNAME = "passlib"

SRC_URI = "https://pypi.python.org/packages/source/p/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "2f872ae7c72ca338634c618f2cff5863"
SRC_URI[sha256sum] = "e987f6000d16272f75314c7147eb015727e8532a3b747b1a8fb58e154c68392d"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools
