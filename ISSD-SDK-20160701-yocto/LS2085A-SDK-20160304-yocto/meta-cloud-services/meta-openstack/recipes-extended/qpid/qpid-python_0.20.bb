DESCRIPTION = "AMQP message brokers"
HOMEPAGE = "http://qpid.apache.org/"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE.txt;md5=b1e01b26bacfc2232046c90a330332b3"
PR = "r0"

SRC_URI = "http://archive.apache.org/dist/qpid/${PV}/${PN}-${PV}.tar.gz"

SRC_URI[md5sum] = "d4dc28a3d68e4b364e54d8fe7f244e5d"
SRC_URI[sha256sum] = "3ca55a5aa11fbbd4e26cb4cdafc9658489c159acadceac60c89d4bfb5c9e1c90"

S = "${WORKDIR}/qpid-${PV}/python"

inherit distutils
