DESCRIPTION = "Python wrapper for extended filesystem attributes"
HOMEPAGE = "http://github.com/xattr/xattr"
SECTION = "devel/python"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE.txt;md5=7ebb582f22ddff5dcb0bc33d04f7cbb8"

PR = "r0"
SRCNAME = "xattr"

SRC_URI = "https://pypi.python.org/packages/source/x/xattr/xattr-0.6.4.tar.gz \
		  "

SRC_URI[md5sum] = "1bef31afb7038800f8d5cfa2f4562b37"
SRC_URI[sha256sum] = "f9dcebc99555634b697fa3dad8ea3047deb389c6f1928d347a0c49277a5c0e9e"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools