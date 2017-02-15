DESCRIPTION = "PAM interface using ctypes"
HOMEPAGE = "http://atlee.ca/software/pam"
SECTION = "devel/python"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://setup.py;beginline=13;endline=13;md5=8ecc573c355c5eb26b2a4a4f3f62684d"

PR = "r0"
SRCNAME = "pam"

SRC_URI = "https://pypi.python.org/packages/source/p/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "4c5247af579352bb6882dac64be10a33"
SRC_URI[sha256sum] = "35e88575afc37a2a5f96e20b22fa55d3e3213370d4ce640af1597c2a1dde226b"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools
