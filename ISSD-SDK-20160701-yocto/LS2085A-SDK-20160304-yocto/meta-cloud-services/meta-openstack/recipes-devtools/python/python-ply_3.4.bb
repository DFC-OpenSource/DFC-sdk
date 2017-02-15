DESCRIPTION = "Python ply: PLY is yet another implementation of lex and yacc for Python"
HOMEPAGE = "https://pypi.python.org/pypi/ply"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://README;beginline=3;endline=30;md5=36197c7ddf450a50a52cf6e743196b1d"

PR = "r0"
SRCNAME = "ply"

SRC_URI = "https://pypi.python.org/packages/source/p/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "ffdc95858819347bf92d7c2acc074894"
SRC_URI[sha256sum] = "af435f11b7bdd69da5ffbc3fecb8d70a7073ec952e101764c88720cdefb2546b"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools
