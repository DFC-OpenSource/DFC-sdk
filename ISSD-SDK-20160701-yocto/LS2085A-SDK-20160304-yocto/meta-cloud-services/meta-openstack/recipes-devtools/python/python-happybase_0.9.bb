DESCRIPTION = "Python library to interact with Apache HBase"
HOMEPAGE = "https://pypi.python.org/pypi/happybase/"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE.rst;md5=41f55ae3d7000e4323e84c468d8b42ee"

PR = "r0"
SRCNAME = "happybase"

SRC_URI = "https://pypi.python.org/packages/source/h/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "0d9ca6e22a7edb03ccd75e99d0acf1ea"
SRC_URI[sha256sum] = "744e342936f6c9e384b203435c38ff21a339626a0d7c11aa177988b5e164ef2a"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

do_install_append() {
	perm_files=`find "${D}${PYTHON_SITEPACKAGES_DIR}/" -name "top_level.txt"`
	for f in $perm_files; do
		chmod 644 "${f}"
	done
}
