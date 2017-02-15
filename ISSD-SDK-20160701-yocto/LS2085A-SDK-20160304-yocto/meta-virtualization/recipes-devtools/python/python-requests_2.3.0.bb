HOMEPAGE = "http://python-requests.org"
SUMMARY = "Python HTTP for Humans."
DESCRIPTION = "\
  Requests is an Apache2 Licensed HTTP library, written in Python, \
  for human beings. \
  .      \
  Most existing Python modules for sending HTTP requests are extremely \
  verbose and cumbersome. Python's builtin urllib2 module provides most \
  of the HTTP capabilities you should need, but the api is thoroughly \
  broken.  It requires an enormous amount of work (even method overrides) \
  to perform the simplest of tasks. \
  .      \
  Things shouldn't be this way. Not in Python \
  "
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=c7869e52c8275537186de35e3cd5f9ec"

PR = "r0"
SRCNAME = "requests"

SRC_URI = "http://pypi.python.org/packages/source/r/requests/${SRCNAME}-${PV}.tar.gz"

SRC_URI[md5sum] = "7449ffdc8ec9ac37bbcd286003c80f00"
SRC_URI[sha256sum] = "1c1473875d846fe563d70868acf05b1953a4472f4695b7b3566d1d978957b8fc"

S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit setuptools
