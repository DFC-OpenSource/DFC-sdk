SUMMARY = "core packagegroup for fsl SDK"
LICENSE = "MIT"

inherit packagegroup

PROVIDES = "${PACKAGES}"

PACKAGES = "\
    packagegroup-fsl-core \
    packagegroup-fsl-core-misc \
    packagegroup-fsl-distro \
    packagegroup-fsl-machine \
    ${@base_contains('DISTRO_FEATURES', 'benchmark', 'packagegroup-fsl-core-benchmark', '',d)} \
    ${@base_contains('DISTRO_FEATURES', 'ext2', 'packagegroup-fsl-core-ext2', '',d)} \
    ${@base_contains('DISTRO_FEATURES', 'mtd', 'packagegroup-fsl-core-mtd', '',d)} \
"

RDEPENDS_packagegroup-fsl-core = "\
    packagegroup-fsl-core-misc \
    packagegroup-fsl-distro \
    packagegroup-fsl-machine \
    ${@base_contains('DISTRO_FEATURES', 'benchmark', 'packagegroup-fsl-core-benchmark', '',d)} \
    ${@base_contains('DISTRO_FEATURES', 'ext2', 'packagegroup-fsl-core-ext2', '',d)} \
    ${@base_contains('DISTRO_FEATURES', 'mtd', 'packagegroup-fsl-core-mtd', '',d)} \
"

DEPENDS_packagegroup-fsl-distro = "${DISTRO_EXTRA_DEPENDS}"
RDEPENDS_packagegroup-fsl-distro = "${DISTRO_EXTRA_RDEPENDS}"
RRECOMMENDS_packagegroup-fsl-distro = "${DISTRO_EXTRA_RRECOMMENDS}"
RDEPENDS_packagegroup-fsl-machine = "${MACHINE_EXTRA_RDEPENDS}"
RRECOMMENDS_packagegroup-fsl-machine = "${MACHINE_EXTRA_RRECOMMENDS}"

RDEPENDS_packagegroup-fsl-core-ext2 = "\
    hdparm \
    e2fsprogs \
    e2fsprogs-badblocks \
    e2fsprogs-e2fsck \
    e2fsprogs-mke2fs \
    e2fsprogs-tune2fs \
"

RDEPENDS_packagegroup-fsl-core-misc = "\
    bash \
    coreutils \
    cryptodev-module \
    devmem2 \
    elfutils \
    ethtool \
    file \
    iproute2 \
    iproute2-tc \
    iptables \
    iputils \
    kernel-modules \
    kmod \
    mdadm \
    merge-files \
    netcat \
    net-tools \
    pkgconfig \
    procps \
    psmisc \
    strongswan \
    sysfsutils \
    sysklogd \
    sysstat \
    util-linux \
"

RDEPENDS_packagegroup-fsl-core-mtd = "\
    mtd-utils \
    mtd-utils-jffs2 \
    mtd-utils-ubifs \
"

RRECOMMENDS_packagegroup-fsl-core-benchmark = "\
    iperf \
    iperf3 \
    iozone3 \
    lmbench \
    netperf \
"

RRECOMMENDS_packagegroup-fsl-core-misc = "\
    bridge-utils \
    i2c-tools \
    inetutils \
    inetutils-ping \
    inetutils-ping6 \
    inetutils-hostname \
    inetutils-ifconfig \
    inetutils-logger \
    inetutils-traceroute \
    inetutils-syslogd \
    inetutils-ftp \
    inetutils-ftpd \
    inetutils-tftp \
    inetutils-tftpd \
    inetutils-telnet \
    inetutils-telnetd \
    inetutils-inetd \
    inetutils-rsh \
    inetutils-rshd \
    ipsec-tools \
    ipsec-demo \
    libhugetlbfs \
    lmsensors-sensors \
    tcpdump \
    tcpreplay \
"
