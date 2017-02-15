require openjdk-8-common.inc

PR = "${INC_PR}.0"

OPENJDK_URI = "\
        hg://hg.openjdk.java.net/aarch64-port;protocol=http;destsuffix=hg/jdk8;name=jdk8;module=jdk8 \
        hg://hg.openjdk.java.net/aarch64-port;protocol=http;destsuffix=hg/corba;name=corba;module=jdk8/corba \
        hg://hg.openjdk.java.net/aarch64-port;protocol=http;destsuffix=hg/hotspot;name=hotspot;module=jdk8/hotspot \
        hg://hg.openjdk.java.net/aarch64-port;protocol=http;destsuffix=hg/jaxp;name=jaxp;module=jdk8/jaxp \
        hg://hg.openjdk.java.net/aarch64-port;protocol=http;destsuffix=hg/jaxws;name=jaxws;module=jdk8/jaxws \
        hg://hg.openjdk.java.net/aarch64-port;protocol=http;destsuffix=hg/jdk;name=jdk;module=jdk8/jdk \
        hg://hg.openjdk.java.net/aarch64-port;protocol=http;destsuffix=hg/langtools;name=langtools;module=jdk8/langtools \
        hg://hg.openjdk.java.net/aarch64-port;protocol=http;destsuffix=hg/nashorn;name=nashorn;module=jdk8/nashorn \
       "

SRCREV_jdk8 = "${AUTOREV}"
SRCREV_corba = "${AUTOREV}"
SRCREV_hotspot = "${AUTOREV}"
SRCREV_jaxp = "${AUTOREV}"
SRCREV_jaxws = "${AUTOREV}"
SRCREV_jdk = "${AUTOREV}"
SRCREV_langtools = "${AUTOREV}"
SRCREV_nashorn = "${AUTOREV}"

S = "${WORKDIR}/jdk8"

LIC_FILES_CHKSUM="file://LICENSE;md5=7b4baeedfe2d40cb03536573bc2c89b1"

BUILD_DIR="linux-aarch64-normal-clientANDserver-release/images"
