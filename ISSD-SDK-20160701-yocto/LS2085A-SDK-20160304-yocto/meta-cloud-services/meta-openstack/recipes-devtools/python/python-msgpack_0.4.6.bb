DESCRIPTION = "MessagePack (de)serializer"
HOMEPAGE = "https://pypi.python.org/pypi/msgpack-python/"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://COPYING;md5=cd9523181d9d4fbf7ffca52eaa2a5751"

PR = "r0"
SRCNAME = "msgpack-python"

SRC_URI = "https://pypi.python.org/packages/source/m/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "8b317669314cf1bc881716cccdaccb30"
SRC_URI[sha256sum] = "bfcc581c9dbbf07cc2f951baf30c3249a57e20dcbd60f7e6ffc43ab3cc614794"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools
