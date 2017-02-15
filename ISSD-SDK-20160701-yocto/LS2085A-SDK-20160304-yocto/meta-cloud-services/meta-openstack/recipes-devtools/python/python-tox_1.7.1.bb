DESCRIPTION = "virtualenv-based automation of test activities"
HOMEPAGE = "http://tox.testrun.org"
SECTION = "devel/python"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=2d0fc2c2c954dc4d41043e67d4a8d8e7"

PR = "r0"

SRCNAME = "tox"
SRC_URI = "http://pypi.python.org/packages/source/t/${SRCNAME}/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "ec85bbfc7bd29600e91aa0e9754645d0"
SRC_URI[sha256sum] = "869cb9e07847a9f0238f5a5029f3621504a5a3ec05af6d878e879b354c6851c4"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools

DEPENDS += " \
        python-pip \
        "

RDEPENDS_${PN} += " \
        python-virtualenv \
        python-py \
        python-pytest \
        "

do_install_append() {

    install -d ${D}/${sysconfdir}/${SRCNAME}
    cp ${S}/tox.ini    ${D}/${sysconfdir}/${SRCNAME}
    cp ${S}/setup.py   ${D}/${sysconfdir}/${SRCNAME}
    cp ${S}/README.rst ${D}/${sysconfdir}/${SRCNAME}

    ln -s ${PYTHON_SITEPACKAGES_DIR}/tox ${D}/${sysconfdir}/${SRCNAME}/tox
}

FILES_${PN} += "${sysconfdir}/${SRCNAME}/*"
