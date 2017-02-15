SUMMARY = "Supports the Python WSGI interface"
DESCRIPTION = "\
  The aim of mod_wsgi is to implement a simple to use Apache module which can host \
  any Python application which supports the Python WSGI interface. The module would \
  be suitable for use in hosting high performance production web sites, as well as \
  your average self managed personal sites running on web hosting services."

HOMEPAGE = "http://code.google.com/p/modwsgi/"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENCE;md5=3b83ef96387f14655fc854ddc3c6bd57"

SRCNAME = "mod_wsgi"
SRC_URI = "\
	http://modwsgi.googlecode.com/files/${SRCNAME}-${PV}.tar.gz \
	file://configure_var.patch \
	file://build-fix-builds-that-have-separated-build-dir.patch \
	"

S = "${WORKDIR}/${SRCNAME}-${PV}"

SRC_URI[md5sum] = "f42d69190ea0c337ef259cbe8d94d985"
SRC_URI[sha256sum] = "ae85c98e9e146840ab3c3e4490e6774f9bef0f99b9f679fca786b2adb5b4b6e8"

inherit autotools distutils-base

DEPENDS += "apache2-native apache2 python-native"
RDEPENDS_${PN} = "python"

EXTRA_OECONF = "\
	--with-apxs=${STAGING_BINDIR_CROSS}/apxs \
	--disable-framework \
	PYTHON_VERSION=${PYTHON_BASEVERSION} \
	PYTHON_INCLUDEPY=-I${STAGING_INCDIR}/python${PYTHON_BASEVERSION} \
	PYTHON_CFLAGS='-DNDEBUG' \
	PYTHON_LIBDIR=${STAGING_LIBDIR} \
	PYTHON_CFGDIR=${STAGING_LIBDIR}/python${PYTHON_BASEVERSION}/config \
	PYTHON_FRAMEWORKDIR='no-framework' \
	PYTHON_FRAMEWORKPREFIX=' ' \
	PYTHON_FRAMEWORK=' ' \
	PYTHON_LIBS='-lpthread -ldl  -lpthread -lutil' \
	PYTHON_SYSLIBS='-lm' \
	"

CFLAGS += " -I${STAGING_INCDIR}/apache2"

FILES_${PN} += "/etc/apache2/"
FILES_${PN}-dbg += "${libdir}/apache2/modules/.debug"

do_install_append() {
	mkdir -p ${D}/etc/apache2/modules.d/
	echo "LoadModule wsgi_module ${libdir}/apache2/modules/mod_wsgi.so" > \
	  ${D}/etc/apache2/modules.d/wsgi.load
}

# to load:
# LoadModule wsgi_module modules/mod_wsgi.so

# Apache/2.2.2 (Unix) mod_wsgi/1.0 Python/2.3 configured
