DESCRIPTION = "Plugin for setuptools that enables git integration"
HOMEPAGE = "https://github.com/wichert/setuptools-git"
SECTION = "devel/python"
LICENSE = "BSD"
LIC_FILES_CHKSUM = "file://LICENSE.txt;md5=3775480a712fc46a69647678acb234cb"

SRCNAME = "setuptools-git"

SRC_URI = "https://pypi.python.org/packages/source/s/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "7b5967e9527c789c3113b07a1f196f6e"
SRC_URI[sha256sum] = "047d7595546635edebef226bc566579d422ccc48a8a91c7d32d8bd174f68f831"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

# conflicting file prevention
do_install_append() {
	rm -f ${D}${libdir}/python*/site-packages/site.py*
}
BBCLASSEXTEND = "native"
