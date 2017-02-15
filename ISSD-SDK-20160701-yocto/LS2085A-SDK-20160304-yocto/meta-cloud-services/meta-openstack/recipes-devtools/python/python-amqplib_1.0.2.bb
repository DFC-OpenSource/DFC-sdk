DESCRIPTION = "Python client for the Advanced Message Queuing Procotol (AMQP)"
HOMEPAGE = "http://code.google.com/p/py-amqplib/"
SECTION = "devel/python"
LICENSE = "LGPL-3.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=1702a92c723f09e3fab3583b165a8d90"

PR = "r1"
SRCNAME = "amqplib"

SRC_URI = "http://py-${SRCNAME}.googlecode.com/files/${SRCNAME}-${PV}.tgz"

SRC_URI[md5sum] = "5c92f17fbedd99b2b4a836d4352d1e2f"
SRC_URI[sha256sum] = "843d69b681a60afd21fbf50f310404ec67fcdf9d13dfcf6e9d41f3b456217e5b"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools
