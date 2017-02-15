SUMMARY = "Pure python memcached client"
DESCRIPTION = "\
  This software is a 100% Python interface to the memcached memory cache daemon. \
  It is the client side software which allows storing values in one or more, possibly remote, \
  memcached servers. Search google for memcached for more information."
HOMEPAGE = "https://pypi.python.org/pypi/python-memcached/"
SECTION = "devel/python"
LICENSE = "PSF"
LIC_FILES_CHKSUM = "file://PSF.LICENSE;md5=7dd786e8594f1e787da94a946557b40e"

PR = "r0"

SRC_URI = "http://ftp.tummy.com/pub/python-memcached/old-releases/${PN}-${PV}.tar.gz"

SRC_URI[md5sum] = "e0e2b44613bb5443e3411b20f16664e0"
SRC_URI[sha256sum] = "af04ea031b271a54f085166773e028fe053fc1d9a58cd8b3c3a57945990bfb48"

inherit setuptools
