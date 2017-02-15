DESCRIPTION = "Command Line Interface Formulation Framework"
HOMEPAGE = "https://github.com/dreamhost/cliff"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=3b83ef96387f14655fc854ddc3c6bd57"

SRCNAME = "cliff"

SRC_URI = "https://pypi.python.org/packages/source/c/${SRCNAME}/${SRCNAME}-${PV}.tar.gz \
    file://0001-Fix-object-has-no-attribute-debug-error.patch \
    "

SRC_URI[md5sum] = "8f3f67d57bf5a2541efeedcea1aa64ab"
SRC_URI[sha256sum] = "209882e199fdf98aeed0db44b922aa688e5a66d81c45148a5743804f9aa680e1"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools rmargparse

DEPENDS += " \
	python-pbr \
	"

RDEPENDS_${PN} += "python-prettytable \
            python-cmd2 \
            python-pyparsing \
            python-pbr \
            "

CLEANBROKEN = "1"
