DESCRIPTION = "functools.singledispatch from Python 3.4 to Python 2.6"
HOMEPAGE = "https://pypi.python.org/pypi/singledistpatch"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://README.rst;md5=8d7cca7c6cfbcc204a6e5ab2b8e8f049"

PR = "r0"
SRCNAME = "singledispatch"

SRC_URI = "https://pypi.python.org/packages/source/s/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "a8e91e6953767614fabb6b6e974fea0e"
SRC_URI[sha256sum] = "4bdd0307cae0d13abb0546df1ab85201b9067090d191e33387e27e1463a7bfd5"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools
