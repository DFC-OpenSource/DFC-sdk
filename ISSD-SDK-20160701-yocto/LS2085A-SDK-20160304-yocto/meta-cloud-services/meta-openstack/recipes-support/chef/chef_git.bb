#
# Copyright (C) 2014 Wind River Systems, Inc.
#
SUMMARY = "Supports the deployment of OpenStack nodes"
DESCRIPTION = "Use chef-solo to help reconfigure and deployment of controller \
and compute nodes. Install script downloaded from this link \
(https://www.opscode.com/chef/install.sh) and the attached archives created from it."

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=8f7bb094c7232b058c7e9f2e431f389c"

PR = "r0"

#SRC_URI[md5sum] = "521d9184648b12e0ea331367d287314f"
#SRC_URI[sha256sum] ="d00874468a4f9e43d6acb0d5238a50ebb4c79f17fd502741a0fc83d76eb253cf"

BPV = "11.12.4"
PV = "${BPV}"
SRCREV = "410af3e88cf9f0793c56363563be8fa173244d3a"

S = "${WORKDIR}/git"

SRC_URI = " \
    git://github.com/opscode/chef.git;branch=11-stable \
    "

inherit ruby

DEPENDS += " \
        ruby-native \
        pry-native \
        yard-native \
        bundler-native \
        "

RDEPENDS_${PN} += " \
        ruby \
        chef-zero \
        coderay \
        diff-lcs \
        erubis \
        hashie \
        highline \
        ipaddress \
        json \
        method-source \
        mime-types \
        mixlib-authentication \
        mixlib-cli \
        mixlib-config \
        mixlib-log \
        mixlib-shellout \
        net-ssh \
        net-ssh-gateway \
        net-ssh-multi \
        ohai \
        pry \
        rack \
        rest-client \
        slop \
        systemu \
        yajl-ruby \
        make \
        "

RUBY_INSTALL_GEMS = "pkg/chef-${BPV}.gem"

do_install_prepend() {
	rake gem
}
