SUMMARY = "A tiny LRU cache implementation and decorator"
DESCRIPTION = "repoze.lru is a LRU (least recently used) cache implementation. \
Keys and values that are not used frequently will be evicted from the cache faster \
than keys and values that are used frequently. \
"
HOMEPAGE = "https://pypi.python.org/pypi/repoze.lru"
SECTION = "devel/python"
LICENSE = "BSD-derived"
LIC_FILES_CHKSUM = "file://LICENSE.txt;md5=2c33cdbc6bc9ae6e5d64152fdb754292"

SRCNAME = "repoze.lru"

SRC_URI = "http://pypi.python.org/packages/source/r/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "2c3b64b17a8e18b405f55d46173e14dd"
SRC_URI[sha256sum] = "0f7a323bf716d3cb6cb3910cd4fccbee0b3d3793322738566ecce163b01bbd31"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools
