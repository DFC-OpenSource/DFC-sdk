HOMEPAGE = "http://www.spice-space.org/page/Html5"
SUMMARY = "Prototype Spice Javascript client"
DESCRIPTION = "\
 Spice Web client which runs entirely within a modern browser. It is \
 limited in function, a bit slow, and lacks support for many features of \
 Spice (audio, video, agents just to name a few). \
 . \
 The Simple Protocol for Independent Computing Environments (SPICE) is \
 a remote display system built for virtual environments which allows \
 you to view a computing 'desktop' environment not only on the machine \
 where it is running, but from anywhere on the Internet and from a wide \
 variety of machine architectures. \
 "
LICENSE = "GPLv3 & LGPLv3"
LIC_FILES_CHKSUM = "\
  file://COPYING;md5=d32239bcb673463ab874e80d47fae504 \
  file://COPYING.LESSER;md5=e6a600fd5e1d9cbde2d983680233ad02"

# Version is based on checkout 0.1.4"
PV = "0.1.4"
SRCREV = "19ade3cf38cc5f5b61fd038f5ce5f4cdb080e9ca"
SRC_URI = "git://anongit.freedesktop.org/spice/spice-html5"

S = "${WORKDIR}/git"

RDEPENDS_${PN} = "python-websockify"

do_install() {
	oe_runmake DESTDIR="${D}" datadir="${D}/${datadir}" install
}

CLEANBROKEN = "1"
