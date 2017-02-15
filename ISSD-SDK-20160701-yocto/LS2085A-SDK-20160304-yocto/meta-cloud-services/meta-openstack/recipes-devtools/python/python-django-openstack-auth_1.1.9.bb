DESCRIPTION = "A Django authentication backend for use with the OpenStack Keystone backend."
HOMEPAGE = "http://django_openstack_auth.readthedocs.org/"
SECTION = "devel/python"
LICENSE = "BSD"
LIC_FILES_CHKSUM = "file://LICENSE;md5=1dece7821bf3fd70fe1309eaa37d52a2"

PR = "r0"
SRCNAME = "django_openstack_auth"

SRC_URI = "https://pypi.python.org/packages/source/d/${SRCNAME}/${SRCNAME}-${PV}.tar.gz \
"

SRC_URI[md5sum] = "a201c7efbf552042f0bae64814454162"
SRC_URI[sha256sum] = "650b3fd528d179a36cf629aa7dad53e03aed6cf9300ea4a337260887e71e9b53"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools 

DEPENDS += " \
        python-pip \
        python-pbr \
        "

RDEPENDS_${PN} += "python-django  \
	python-keystoneclient  \
	python-pbr \
	"
