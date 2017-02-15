DESCRIPTION = "hybrid SQL + LDAP backend for openstack keystone"
HOMEPAGE = "https://github.com/SUSE-Cloud/keystone-hybrid-backend"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://hybrid_identity.py;beginline=1;endline=14;md5=06c14f61d807564e89a36bc1b33465d5"

PR = "r0"

SRC_URI = "git://github.com/SUSE-Cloud/keystone-hybrid-backend.git;branch=havana"

PV="git${SRCPV}"
SRCREV="0bd376242f8522edef7031d2339b9533b86c17aa"
S = "${WORKDIR}/git"

inherit python-dir

do_install_append() {
    install -D -m 0644 hybrid_assignment.py ${D}/${PYTHON_SITEPACKAGES_DIR}/keystone/assignment/backends/hybrid_assignment.py
    install -D -m 0644 hybrid_identity.py ${D}/${PYTHON_SITEPACKAGES_DIR}/keystone/identity/backends/hybrid_identity.py
}

FILES_${PN} += "${PYTHON_SITEPACKAGES_DIR}"
