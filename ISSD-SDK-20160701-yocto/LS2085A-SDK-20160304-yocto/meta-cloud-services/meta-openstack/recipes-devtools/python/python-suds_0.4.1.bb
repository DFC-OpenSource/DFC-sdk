DESCRIPTION = "Lightweight SOAP client"
HOMEPAGE = "https://fedorahosted.org/suds/"
SECTION = "devel/python"
LICENSE = "LGPLv3"
LIC_FILES_CHKSUM = "file://LICENSE;md5=847e96bce86d8774f491a92924343a29"

PR = "r0"
SRCNAME = "suds"

SRC_URI = "https://fedorahosted.org/releases/s/u/${SRCNAME}/${PN}-${PV}.tar.gz"

SRC_URI[md5sum] = "95a2f04378931e973cbb3cca8f8d9765"
SRC_URI[sha256sum] = "dd711c2635483733cd3aebf5073edf338595a2d2cae1398041f0273e9bdaac66"

inherit setuptools
