DESCRIPTION = "A comprehensive, fast, pure Python memcached client"
HOMEPAGE = "https://github.com/Pinterest/pymemcache"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://PKG-INFO;md5=42205da10d12fff27726c08f588b1a0b"

PR = "r0"

SRCNAME = "pymemcache"
SRC_URI = "http://pypi.python.org/packages/source/p/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "56dcfa6dfa7205118b60531e5336ab8b"
SRC_URI[sha256sum] = "d8b3d52909eedf975f21236b099510866df8f6cf3d68f67f763609fe2c5ea78b"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

# DEPENDS_default: python-pip

DEPENDS += " \
        python-pip \
        python-nose \
        "

# RDEPENDS_default: 
RDEPENDS_${PN} += " \
        python-nose \
        "
