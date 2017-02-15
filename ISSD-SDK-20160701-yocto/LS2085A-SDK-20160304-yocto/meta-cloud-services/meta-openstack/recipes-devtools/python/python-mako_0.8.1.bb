DESCRIPTION = "A super-fast templating language that borrows the  best ideas from the existing templating languages."
HOMEPAGE = "http://www.makotemplates.org/"
SECTION = "devel/python"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=da8dd26ed9751ee0cfdf9df1a16bbb54"

PR = "r0"
SRCNAME = "Mako"

SRC_URI = "https://pypi.python.org/packages/source/M/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "96d962464ce6316004af0cc48495d73e"
SRC_URI[sha256sum] = "4791be305338b1fbe09054ec42fb606856599cdcdcde6f348858c13b5fa29158"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools
