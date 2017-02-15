DESCRIPTION = "WSGI request and response object"
HOMEPAGE = "http://webob.org/"
SECTION = "devel/python"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://docs/license.txt;md5=8ed3584bcc78c16da363747ccabc5af5"

PR = "r0"
SRCNAME = "WebOb"

SRC_URI = "http://pypi.python.org/packages/source/W/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "11825b7074ba7043e157805e4e6e0f55"
SRC_URI[sha256sum] = "325c249f3ac35e72b75ba13b2c60317def0c986a24a413ebf700509ea4c73a13"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

RDEPENDS_${PN} += " \
	python-sphinx \
	python-nose \
	"

