DESCRIPTION = "Python bindings for the Apache Thrift RPC system"
HOMEPAGE = "https://pypi.python.org/pypi/amqp/"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://README;md5=1bd6aa02231f9e3aa626d8c13c20e1c8"

PR = "r0"
SRCNAME = "thrift"

SRC_URI = "https://pypi.python.org/packages/source/t/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "8989a8a96b0e3a3380cfb89c44e172a6"
SRC_URI[sha256sum] = "7d1a75c9bd73b02662483fc68b3e60bc24d75f6e55492d379f3053d68c937770"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools
