DESCRIPTION = "Compresses linked and inline JavaScript or CSS into single cached files."
HOMEPAGE = "http://django-compressor.readthedocs.org/en/latest/"
SECTION = "devel/python"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=9f8de96acfc347c41ce7c15f2e918ef6"

PR = "r0"
SRCNAME = "django_compressor"

SRC_URI = "https://pypi.python.org/packages/source/d/${SRCNAME}/${SRCNAME}-${PV}.tar.gz \
"

SRC_URI[md5sum] = "dc050f1a85f83f927f94bcb09e8bdd0f"
SRC_URI[sha256sum] = "b26034230efcef7d60e5267890eda656dfc49c567f27125d907eee4fe7f9a6ec"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools 

RDEPENDS_${PN} += "python-django-appconf  \
	"

CLEANBROKEN = "1"
