DESCRIPTION = "Java runtime based upon the OpenJDK8"
HOMEPAGE = "http://openjdk.java.net/projects/jdk8"
LICENSE  = "GPL-2.0-with-classpath-exception"
PRIORITY = "optional"
SECTION  = "libs"
INHIBIT_PACKAGE_STRIP = "1"
#INHIBIT_PACKAGE_DEBUG_SPLIT = "1"

INC_PR = "r16"

SRC_URI = " \
  ${OPENJDK_URI} \
  "

libdir_jvm ?= "${libdir}/jvm"

JDKPN = "openjdk-8"
JDK_DIR = "java-8-openjdk"
JDK_HOME = "${libdir_jvm}/${JDK_DIR}"

PN = "${JDKPN}-jre"
PROVIDES += "${JDKPN}"

DEPENDS = " \
	   zip-native \
	   zlib \
	   jpeg libpng giflib \
	   gtk+ glib-2.0 \
	   cups fontconfig \
	   libxt libxinerama libxrender libxtst libxi \
	   freetype alsa-lib libffi libxt \
	  "

# No package should directly depend on this (it should require
# java2-runtime instead).
PRIVATE_LIBS = "\
	libunpack.so libverify.so libjava.so libzip.so libnpt.so \
	libjava_crw_demo.so libhprof.so libnet.so libnio.so \
	libmanagement.so libinstrument.so libjsound.so libjsoundalsa.so \
	libj2pcsc.so libj2pkcs11.so libj2gss.so libmlib_image.so \
	libawt.so libsplashscreen.so libfreetype.so.6 libfontmanager.so \
	libjpeg.so liblcms.so librmi.so libjawt.so libjaas_unix.so \
	libattach.so libjdwp.so libdt_socket.so libhpi.so libjli.so \
	libmawt.so libjvm.so \
	libversionCheck.so libcompiledMethodLoad.so libgctest.so \
	libheapViewer.so libheapTracker.so libminst.so libmtrace.so \
	libwaiters.so libhprof.so \
       "

COMPATIBLE_HOST = "(aarch64).*-linux"
JAVA_HOME[unexport] = "1"
S="${WORKDIR}/openjdk8-aarch64-port-snapshot"

# The compiler tests in do_configure seem to drop TARGET_CC_ARCH from $CC, so add it into CFLAGS.
CFLAGS =+ "${TARGET_CC_ARCH} ${TOOLCHAIN_OPTIONS}"
CXXFLAGS =+ "${TARGET_CC_ARCH} ${TOOLCHAIN_OPTIONS}"
export EXTRA_CFLAGS = "${TOOLCHAIN_OPTIONS}"
ASNEEDED = ""

do_configure() {
	unset MAKE
	bash ./configure --with-debug-level=release --with-jvm-variants=client,server --with-sys-root=${STAGING_DIR_TARGET} --openjdk-target=${TARGET_SYS} --with-zlib=system --with-stdc++lib=dynamic --disable-ccache --disable-zip-debug-info
}

do_compile() {
	make \
		LOG=debug \
		ALT_SDT_H=/openjdk/path/to/a/nonexistent/directory \
		EXTRA_CFLAGS='${EXTRA_CFLAGS}' \
		CC='${CC}' \
		CXX='${CXX}' \
		LD='${CC} ${LDFLAGS}' \
		LDCXX='${CXX} ${LDFLAGS}' \
		LDEXE='${CC} ${LDFLAGS}' \
		LDEXECXX='${CXX} ${LDFLAGS}' \
		images

}

do_install() {
	install -d ${D}${libdir_jvm}
	install -d ${D}${JDK_HOME}/jtreg
	install -d ${D}${JDK_HOME}/jtreg/hotspot
	install -d ${D}${JDK_HOME}/jtreg/jdk
	cp -R ${S}/build/${BUILD_DIR}/j2sdk-image/* ${D}${JDK_HOME}
	cp -R ${S}/hotspot/test ${D}${JDK_HOME}/jtreg/hotspot
	cp -R ${S}/jdk/test ${D}${JDK_HOME}/jtreg/jdk
# The OpenJDK build does its own thing with debuginfo files; as we
# default to a 'release' build we remove these files.
	find ${D}${JDK_HOME} -name \*.debuginfo -print | xargs rm
# The upstream Mercurial repo has various incompatible binaries checked in.
# Remove these otherwise bitbake complains about mismatched architecture, et al.
	find ${D}${JDK_HOME}/jtreg -name \*.so -print | xargs rm
	rm ${D}${JDK_HOME}/jtreg/jdk/test/sun/management/jmxremote/bootstrap/linux-i586/launcher
	rm ${D}${JDK_HOME}/jtreg/jdk/test/sun/management/jmxremote/bootstrap/linux-amd64/launcher
	rm ${D}${JDK_HOME}/jtreg/jdk/test/sun/management/jmxremote/bootstrap/solaris-sparcv9/launcher
	rm ${D}${JDK_HOME}/jtreg/jdk/test/sun/management/jmxremote/bootstrap/solaris-amd64/launcher
	chmod -R u+rw,go+r ${D}${JDK_HOME}
}

PACKAGES = " \
	    ${JDKPN}-jdk \
	    ${JDKPN}-doc \
	    ${JDKPN}-source \
	    ${JDKPN}-demo \
	    ${JDKPN}-jtreg \
	    ${JDKPN}-jre \
	   "

FILES_${JDKPN}-jdk = " \
		       ${JDK_HOME}/bin \
		       ${JDK_HOME}/lib \
		       ${JDK_HOME}/include \
		       ${JDK_HOME}/ASSEMBLY_EXCEPTION \
		       ${JDK_HOME}/release \
		       ${JDK_HOME}/THIRD_PARTY_README \
		       ${JDK_HOME}/LICENSE \
		      "

FILES_${JDKPN}-doc = " \
		       ${JDK_HOME}/man \
		      "

FILES_${JDKPN}-source = " \
		          ${JDK_HOME}/src.zip \
			"

FILES_${JDKPN}-demo = " \
		        ${JDK_HOME}/demo \
                        ${JDK_HOME}/sample \
                      "

FILES_${JDKPN}-jtreg = " \
                         ${JDK_HOME}/jtreg \
                      "

FILES_${PN} = " \
	        ${JDK_HOME}/jre \
              "

INSANE_SKIP_${JDKPN}-jdk = "dev-so"
INSANE_SKIP_${JDKPN}-jre = "dev-so"

RPROVIDES_${JDKPN}-jre = "java2-runtime java2-vm"
RPROVIDES_${JDKPN}-jdk = "java2-runtime java2-vm"
RDEPENDS_${JDKPN}-demo = "java2-runtime"

RRECOMMENDS_${JDKPN}-jre = "liberation-fonts"
RRECOMMENDS_${JDKPN}-jdk = "liberation-fonts"

inherit update-alternatives

pkg_postinst_${JDKPN}-jre () {
    update-alternatives --install ${bindir}/java java ${JDK_HOME}/jre/bin/java 75
    update-alternatives --install ${bindir}/jjs jjs ${JDK_HOME}/jre/bin/jjs 75
    update-alternatives --install ${bindir}/keytool keytool ${JDK_HOME}/jre/bin/keytool 75
    update-alternatives --install ${bindir}/orbd orbd ${JDK_HOME}/jre/bin/orbd 75
    update-alternatives --install ${bindir}/pack200 pack200 ${JDK_HOME}/jre/bin/pack200 75
    update-alternatives --install ${bindir}/policytool policytool ${JDK_HOME}/jre/bin/policytool 75
    update-alternatives --install ${bindir}/rmid rmid ${JDK_HOME}/jre/bin/rmid 75
    update-alternatives --install ${bindir}/rmiregistry rmiregistry ${JDK_HOME}/jre/bin/rmiregistry 75
    update-alternatives --install ${bindir}/servertool servertool ${JDK_HOME}/jre/bin/servertool 75
    update-alternatives --install ${bindir}/tnameserv tnameserv ${JDK_HOME}/jre/bin/tnameserv 75
    update-alternatives --install ${bindir}/unpack200 unpack200 ${JDK_HOME}/jre/bin/unpack200 75
}

pkg_prerm_${JDKPN}-jre () {
    update-alternatives --remove java ${JDK_HOME}/jre/bin/java
    update-alternatives --remove jjs ${JDK_HOME}/jre/bin/jjs
    update-alternatives --remove keytool ${JDK_HOME}/jre/bin/keytool
    update-alternatives --remove orbd ${JDK_HOME}/jre/bin/orbd
    update-alternatives --remove pack200 ${JDK_HOME}/jre/bin/pack200
    update-alternatives --remove policytool ${JDK_HOME}/jre/bin/policytool
    update-alternatives --remove rmid ${JDK_HOME}/jre/bin/rmid
    update-alternatives --remove rmiregistry ${JDK_HOME}/jre/bin/rmiregistry
    update-alternatives --remove servertool ${JDK_HOME}/jre/bin/servertool
    update-alternatives --remove tnameserv ${JDK_HOME}/jre/bin/tnameserv
    update-alternatives --remove unpack200 ${JDK_HOME}/jre/bin/unpack200
}

pkg_postinst_${JDKPN}-jdk () {
    update-alternatives --install ${bindir}/appletviewer appletviewer ${JDK_HOME}/bin/appletviewer 100
    update-alternatives --install ${bindir}/extcheck extcheck ${JDK_HOME}/bin/extcheck 100
    update-alternatives --install ${bindir}/idlj idlj ${JDK_HOME}/bin/idlj 100
    update-alternatives --install ${bindir}/jar jar ${JDK_HOME}/bin/jar 100
    update-alternatives --install ${bindir}/jarsigner jarsigner ${JDK_HOME}/bin/jarsigner 100
    update-alternatives --install ${bindir}/java java ${JDK_HOME}/bin/java 100
    update-alternatives --install ${bindir}/javac javac ${JDK_HOME}/bin/javac 100
    update-alternatives --install ${bindir}/javadoc javadoc ${JDK_HOME}/bin/javadoc 100
    update-alternatives --install ${bindir}/javah javah ${JDK_HOME}/bin/javah 100
    update-alternatives --install ${bindir}/javap javap ${JDK_HOME}/bin/javap 100
    update-alternatives --install ${bindir}/java-rmi.cgi java-rmi.cgi ${JDK_HOME}/bin/java-rmi.cgi 100
    update-alternatives --install ${bindir}/jcmd jcmd ${JDK_HOME}/bin/jcmd 100
    update-alternatives --install ${bindir}/jconsole jconsole ${JDK_HOME}/bin/jconsole 100
    update-alternatives --install ${bindir}/jdb jdb ${JDK_HOME}/bin/jdb 100
    update-alternatives --install ${bindir}/jdeps jdeps ${JDK_HOME}/bin/jdeps 100
    update-alternatives --install ${bindir}/jhat jhat ${JDK_HOME}/bin/jhat 100
    update-alternatives --install ${bindir}/jinfo jinfo ${JDK_HOME}/bin/jinfo 100
    update-alternatives --install ${bindir}/jjs jjs ${JDK_HOME}/bin/jjs 100
    update-alternatives --install ${bindir}/jmap jmap ${JDK_HOME}/bin/jmap 100
    update-alternatives --install ${bindir}/jps jps ${JDK_HOME}/bin/jps 100
    update-alternatives --install ${bindir}/jrunscript jrunscript ${JDK_HOME}/bin/jrunscript 100
    update-alternatives --install ${bindir}/jsadebugd jsadebugd ${JDK_HOME}/bin/jsadebugd 100
    update-alternatives --install ${bindir}/jstack jstack ${JDK_HOME}/bin/jstack 100
    update-alternatives --install ${bindir}/jstat jstat ${JDK_HOME}/bin/jstat 100
    update-alternatives --install ${bindir}/jstatd jstatd ${JDK_HOME}/bin/jstatd 100
    update-alternatives --install ${bindir}/keytool keytool ${JDK_HOME}/bin/keytool 100
    update-alternatives --install ${bindir}/native2ascii native2ascii ${JDK_HOME}/bin/native2ascii 100
    update-alternatives --install ${bindir}/orbd orbd ${JDK_HOME}/bin/orbd 100
    update-alternatives --install ${bindir}/pack200 pack200 ${JDK_HOME}/bin/pack200 100
    update-alternatives --install ${bindir}/policytool policytool ${JDK_HOME}/bin/policytool 100
    update-alternatives --install ${bindir}/rmic rmic ${JDK_HOME}/bin/rmic 100
    update-alternatives --install ${bindir}/rmid rmid ${JDK_HOME}/bin/rmid 100
    update-alternatives --install ${bindir}/rmiregistry rmiregistry ${JDK_HOME}/bin/rmiregistry 100
    update-alternatives --install ${bindir}/schemagen schemagen ${JDK_HOME}/bin/schemagen 100
    update-alternatives --install ${bindir}/serialver serialver ${JDK_HOME}/bin/serialver 100
    update-alternatives --install ${bindir}/servertool servertool ${JDK_HOME}/bin/servertool 100
    update-alternatives --install ${bindir}/tnameserv tnameserv ${JDK_HOME}/bin/tnameserv 100
    update-alternatives --install ${bindir}/unpack200 unpack200 ${JDK_HOME}/bin/unpack200 100
    update-alternatives --install ${bindir}/wsgen wsgen ${JDK_HOME}/bin/wsgen 100
    update-alternatives --install ${bindir}/wsimport wsimport ${JDK_HOME}/bin/wsimport 100
    update-alternatives --install ${bindir}/xjc xjc ${JDK_HOME}/bin/xjc 100
}

pkg_prerm_${JDKPN}-jdk () {
    update-alternatives --remove appletviewer ${JDK_HOME}/bin/appletviewer
    update-alternatives --remove extcheck ${JDK_HOME}/bin/extcheck
    update-alternatives --remove idlj ${JDK_HOME}/bin/idlj
    update-alternatives --remove jar ${JDK_HOME}/bin/jar
    update-alternatives --remove jarsigner ${JDK_HOME}/bin/jarsigner
    update-alternatives --remove java ${JDK_HOME}/bin/java
    update-alternatives --remove javac ${JDK_HOME}/bin/javac
    update-alternatives --remove javadoc ${JDK_HOME}/bin/javadoc
    update-alternatives --remove javah ${JDK_HOME}/bin/javah
    update-alternatives --remove javap ${JDK_HOME}/bin/javap
    update-alternatives --remove java-rmi.cgi ${JDK_HOME}/bin/java-rmi.cgi
    update-alternatives --remove jcmd ${JDK_HOME}/bin/jcmd
    update-alternatives --remove jconsole ${JDK_HOME}/bin/jconsole
    update-alternatives --remove jdb ${JDK_HOME}/bin/jdb
    update-alternatives --remove jdeps ${JDK_HOME}/bin/jdeps
    update-alternatives --remove jhat ${JDK_HOME}/bin/jhat
    update-alternatives --remove jinfo ${JDK_HOME}/bin/jinfo
    update-alternatives --remove jjs ${JDK_HOME}/bin/jjs
    update-alternatives --remove jmap ${JDK_HOME}/bin/jmap
    update-alternatives --remove jps ${JDK_HOME}/bin/jps
    update-alternatives --remove jrunscript ${JDK_HOME}/bin/jrunscript
    update-alternatives --remove jsadebugd ${JDK_HOME}/bin/jsadebugd
    update-alternatives --remove jstack ${JDK_HOME}/bin/jstack
    update-alternatives --remove jstat ${JDK_HOME}/bin/jstat
    update-alternatives --remove jstatd ${JDK_HOME}/bin/jstatd
    update-alternatives --remove keytool ${JDK_HOME}/bin/keytool
    update-alternatives --remove native2ascii ${JDK_HOME}/bin/native2ascii
    update-alternatives --remove orbd ${JDK_HOME}/bin/orbd
    update-alternatives --remove pack200 ${JDK_HOME}/bin/pack200
    update-alternatives --remove policytool ${JDK_HOME}/bin/policytool
    update-alternatives --remove rmic ${JDK_HOME}/bin/rmic
    update-alternatives --remove rmid ${JDK_HOME}/bin/rmid
    update-alternatives --remove rmiregistry ${JDK_HOME}/bin/rmiregistry
    update-alternatives --remove schemagen ${JDK_HOME}/bin/schemagen
    update-alternatives --remove serialver ${JDK_HOME}/bin/serialver
    update-alternatives --remove servertool ${JDK_HOME}/bin/servertool
    update-alternatives --remove tnameserv ${JDK_HOME}/bin/tnameserv
    update-alternatives --remove unpack200 ${JDK_HOME}/bin/unpack200
    update-alternatives --remove wsgen ${JDK_HOME}/bin/wsgen
    update-alternatives --remove wsimport ${JDK_HOME}/bin/wsimport
    update-alternatives --remove xjc ${JDK_HOME}/bin/xjc
}
