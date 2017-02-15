DESCRIPTION = "Low-level AMQP client for Python"
HOMEPAGE = "https://pypi.python.org/pypi/amqp/"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=1702a92c723f09e3fab3583b165a8d90"

SRCNAME = "amqp"

SRC_URI = "https://pypi.python.org/packages/source/a/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "a061581b6864f838bffd62b6a3d0fb9f"
SRC_URI[sha256sum] = "ebcfc867de5a68f9f5ba14d11dbad88e6aff8435a8d39339d5ceb0e5b06de640"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools
