DEPENDS = " \
binutils \
bison-native \
boost \
bzip2 \
cmake \
elfutils \
expat \
flex-native \
gd \
glog \
icu \
libcap \
uw-imap \
libdwarf \
libevent-fb \
libmcrypt \
libmemcached \
libunwind \
libxml2 \
mysql5 \
ncurses \
onig \
openldap \
openssl \
libpam \
pcre \
readline \
zlib \
tbb \
${EXTRA_DEPENDS} \
"

# optional (for now) dependencies:
EXTRA_DEPENDS = "gperftools"
EXTRA_DEPENDS_aarch64 = ""

# 64-bit platforms only
COMPATIBLE_HOST = '(x86_64.*|aarch64.*)-linux'

LICENSE = "PHP & Zend"

LIC_FILES_CHKSUM = " \
		file://LICENSE.PHP;md5=cb564efdf78cce8ea6e4b5a4f7c05d97 \
		file://LICENSE.ZEND;md5=69e7a9c51846dd6692f1b946f95f6c60"

SRC_URI = "git://github.com/facebook/hhvm.git \
           file://hrw-check-for-libdwarf-in-our-place-first.patch \
           "

SRCREV = "4c4d11304aef8857dcce8524e7fd9223e00191b5"

PV = "2.0.2+git${SRCPV}"

S = "${WORKDIR}/git"

do_configure_prepend() {
	export HPHP_HOME="${B}"
	export HPHP_LIB="${B}"/bin
	export USE_HHVM=1
	export BOOST_INCLUDEDIR=${STAGING_INCDIR}
	export BOOST_LIBRARYDIR=${STAGING_LIBDIR}
	export LIBGLOG_INCLUDE_DIR=${STAGING_INCDIR}
	export LIBGLOG_LIBRARY=${STAGING_LIBDIR}
}

inherit cmake
