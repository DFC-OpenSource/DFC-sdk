SUMMARY = "Lightweight media scanner meant to be used in not-so-powerful devices"
AUTHOR = "ProFUSION"
HOMEPAGE = "http://lms.garage.maemo.org/"
SECTION = "libs/multimedia"
LICENSE = "LGPLv2.1+"
LIC_FILES_CHKSUM = "file://COPYING;md5=a6f89e2100d9b6cdffcea4f398e37343"
DEPENDS = "sqlite3 flac virtual/libiconv"

PR = "r2"

SRC_URI = "https://github.com/profusion/lightmediascanner/archive/release_${PV}.tar.gz \
"
SRC_URI[md5sum] = "0c758d40eb70288a2c714dfb44c35171"
SRC_URI[sha256sum] = "1bbc28fbe5efbc24aa4eac9e43bbb178a1669de1418afee47b0c6898aff53433"

S = "${WORKDIR}/lightmediascanner-release_${PV}"

PACKAGECONFIG ??= ""
PACKAGECONFIG[ogg] = "--enable-ogg,--disable-ogg,libvorbis"

inherit autotools pkgconfig

do_install_append() {
    install -d ${D}/${bindir}/
    install -m 755 ${B}/src/bin/.libs/test  ${D}/${bindir}/test-lms
}

PACKAGES =+ "${PN}-test"

FILES_${PN}-test = "${bindir}/test-lms"

FILES_${PN}-dbg += "${libdir}/${PN}/plugins/.debug"
FILES_${PN}-staticdev += "${libdir}/${PN}/plugins/*.a"

# otherwise fails with ERROR: could not add conversion charset 'UTF-16BE': Invalid argument
RDEPENDS_${PN}-test_append_libc-glibc = " glibc-gconv-utf-16"

# png.so jpeg.so id3.so are provided also by imlib2-loaders
PRIVATE_LIBS_${PN} = "video-dummy.so png.so jpeg.so id3.so pls.so audio-dummy.so rm.so ogg.so dummy.so m3u.so flac.so asf.so"
