LICENSE = "GPL-2"
LIC_FILES_CHKSUM = "file://COPYING;md5=d7810fab7487fb0aad327b76f1be7cd7"

PV = "4.2"

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"
SRC_URI = "http://snapshot.debian.org/archive/debian/20150926T032755Z/pool/main/l/linux-tools/linux-tools_${PV}.orig.tar.xz"

SRC_URI[md5sum] = "9a4a83c22368281b85e4ab12dcc3ec95"
SRC_URI[sha256sum] = "78b336fca5e250f205fe5bc10cc77943dc95c2d66899f0bc95963b3aab8ef644"

S = "${WORKDIR}/linux-tools-${PV}"
B = "${WORKDIR}/linux-tools-${PV}-build"

do_compile_prepend() {
    mkdir -p ${S}/include/generated
    echo "#define UTS_RELEASE \"${PV}\"" > ${S}/include/generated/utsrelease.h
}

# Ensure the above tarball gets fetched, unpackaged and patched
python () {
	d.delVarFlag("do_fetch", "noexec")
	d.delVarFlag("do_unpack", "noexec")
	d.delVarFlag("do_patch", "noexec")
}
