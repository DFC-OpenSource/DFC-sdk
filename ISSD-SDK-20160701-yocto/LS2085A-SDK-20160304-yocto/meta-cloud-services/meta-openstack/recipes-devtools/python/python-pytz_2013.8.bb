DESCRIPTION = "World timezone definitions, modern and historical"
HOMEPAGE = "http://pytz.sourceforge.net"
SECTION = "devel/python"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE.txt;md5=22b38951eb857cf285a4560a914b7cd6"

PR = "r0"
SRCNAME = "pytz"

SRC_URI = "https://pypi.python.org/packages/source/p/${SRCNAME}/${SRCNAME}-${PV}.tar.gz \
"

SRC_URI[md5sum] = "37750ca749ed3a52523b9682b0b7e381"
SRC_URI[sha256sum] = "58552e870aa2c0a1fa3b4ef923f00fbf3e55afaa87f8d31244d44f188de4793a"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools 
