HOMEPAGE = "http://www.docker.com"
SUMMARY = "Linux container runtime"
DESCRIPTION = "Linux container runtime \
 Docker complements kernel namespacing with a high-level API which \
 operates at the process level. It runs unix processes with strong \
 guarantees of isolation and repeatability across servers. \
 . \
 Docker is a great building block for automating distributed systems: \
 large-scale web deployments, database clusters, continuous deployment \
 systems, private PaaS, service-oriented architectures, etc. \
 . \
 This package contains the daemon and client. Using docker.io on non-amd64 \
 hosts is not supported at this time. Please be careful when using it \
 on anything besides amd64. \
 . \
 Also, note that kernel version 3.8 or above is required for proper \
 operation of the daemon process, and that any lower versions may have \
 subtle and/or glaring issues. \
 "

SRCREV = "2243e32cbbf1c9809c262a7376d34ca43a7a36dc"
SRC_URI = "\
	git://github.com/docker/docker.git \
	file://docker.service \
	file://docker.init \
	file://hi.Dockerfile \
	file://disable_sha1sum_startup.patch \
	"

# The golang-cross embeds a compiler invocation for the pre-parser
# that is incompatible with the $CC definition used by the compiler
# templates so disable CCACHE
CCACHE = ""

# Apache-2.0 for docker
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=1cc0497778922bfd6cb48721deb80dc7"

S = "${WORKDIR}/git"

DOCKER_VERSION = "1.5.0"
PV = "${DOCKER_VERSION}+git${SRCREV}"

DEPENDS = "golang-cross \
    go-cli \
    go-pty \
    go-context \
    go-mux \
    go-patricia \
    go-net \
    go-sqlite \
    go-libtrust \
    go-logrus \
    go-fsnotify \
    go-dbus \
    go-capability \
    go-systemd \
    btrfs-tools \
    sqlite3 \
    "

DEPENDS_append_class-target = "lvm2"
RDEPENDS_${PN} = "curl aufs-util git cgroup-lite util-linux"
RRECOMMENDS_${PN} = "lxc docker-registry rt-tests"
DOCKER_PKG="github.com/docker/docker"

do_configure() {
}

do_compile() {
	export GOARCH="${TARGET_ARCH}"
	# supported amd64, 386, arm
	if [ "${TARGET_ARCH}" = "x86_64" ]; then
		export GOARCH="amd64"
	fi

	# Set GOPATH. See 'PACKAGERS.md'. Don't rely on
	# docker to download its dependencies but rather
	# use dependencies packaged independently.
	cd ${S}
	rm -rf .gopath
	mkdir -p .gopath/src/"$(dirname "${DOCKER_PKG}")"
	ln -sf ../../../.. .gopath/src/"${DOCKER_PKG}"
	export GOPATH="${S}/.gopath:${S}/vendor:${STAGING_DIR_TARGET}/${prefix}/local/go"
	cd -

	# Pass the needed cflags/ldflags so that cgo
	# can find the needed headers files and libraries
	export CGO_CFLAGS="${BUILD_CFLAGS}"
	export CGO_LDFLAGS="${BUILD_LDFLAGS}"

	# this is the unsupported built structure
	# that doesn't rely on an existing docker
	# to build this:
	DOCKER_GITCOMMIT="${SRCREV}" \
	  ./hack/make.sh dynbinary

	# make nsinit from libcontainer
	go install github.com/docker/libcontainer/nsinit/
}

inherit systemd update-rc.d

SYSTEMD_PACKAGES = "${@base_contains('DISTRO_FEATURES','systemd','${PN}','',d)}"
SYSTEMD_SERVICE_${PN} = "${@base_contains('DISTRO_FEATURES','systemd','docker.service','',d)}"

INITSCRIPT_PACKAGES += "${@base_contains('DISTRO_FEATURES','sysvinit','${PN}','',d)}"
INITSCRIPT_NAME_${PN} = "${@base_contains('DISTRO_FEATURES','sysvinit','docker.init','',d)}"
INITSCRIPT_PARAMS_${PN} = "${OS_DEFAULT_INITSCRIPT_PARAMS}"

do_install() {
	mkdir -p ${D}/${bindir}
	cp ${S}/bundles/${DOCKER_VERSION}-dev/dynbinary/docker-${DOCKER_VERSION}-dev \
	  ${D}/${bindir}/docker
	cp ${S}/bundles/${DOCKER_VERSION}-dev/dynbinary/dockerinit-${DOCKER_VERSION}-dev \
	  ${D}/${bindir}/dockerinit

	if ${@base_contains('DISTRO_FEATURES','systemd','true','false',d)}; then
		install -d ${D}${systemd_unitdir}/system
		install -m 644 ${S}/contrib/init/systemd/docker.* ${D}/${systemd_unitdir}/system
		# replaces one copied from above with one that uses the local registry for a mirror
		install -m 644 ${WORKDIR}/docker.service ${D}/${systemd_unitdir}/system
        else
            install -d ${D}${sysconfdir}/init.d
            install -m 0755 ${WORKDIR}/docker.init ${D}${sysconfdir}/init.d/docker.init
	fi

	cp ${S}/vendor/bin/nsinit ${D}/${bindir}

	mkdir -p ${D}/usr/share/docker/
	cp ${WORKDIR}/hi.Dockerfile ${D}/usr/share/docker/
}

inherit useradd
USERADD_PACKAGES = "${PN}"
GROUPADD_PARAM_${PN} = "-r docker"

FILES_${PN} += "/lib/systemd/system/*"

# DO NOT STRIP docker and dockerinit!!!
#
# Reason:
# The "docker" package contains two binaries: "docker" and "dockerinit",
# which are both written in Go. The "dockerinit" package is built first,
# then its checksum is given to the build process compiling the "docker"
# binary. Hence the checksum of the unstripped "dockerinit" binary is hard
# coded into the "docker" binary. At runtime the "docker" binary invokes
# the "dockerinit" binary, but before doing that it ensures the checksum
# of "dockerinit" matches with the hard coded value.
#
INHIBIT_PACKAGE_STRIP = "1"
