DESCRIPTION = "A library that provides a generic versioned object model that is RPC-friendly, with inbuilt serialization, field typing, and remotable method calls"
HOMEPAGE = "http://docs.openstack.org/developer/oslo.versionedobjects/"
SECTION = "devel/python"
LICENSE = "Apache-2"
LIC_FILES_CHKSUM = "file://LICENSE;md5=1dece7821bf3fd70fe1309eaa37d52a2"

SRC_URI[md5sum] = "11309423d1a08aab4a81a20e2dcca8d5"
SRC_URI[sha256sum] = "dac8ef3ec789bd3f69f3f974e9f40282a411d804663eefd1ae39721f16a4c2da"

inherit pypi

RDEPENDS_${PN} += " \
    python-babel \
    python-fixtures \
    python-iso8601 \
    python-mock \
    python-oslo.concurrency \
    python-oslo.context \
    python-oslo.i18n \
    python-oslo.log \
    python-oslo.messaging \
    python-oslo.serialization \
    python-oslo.utils \
    python-webob \
    python-six \
    "
