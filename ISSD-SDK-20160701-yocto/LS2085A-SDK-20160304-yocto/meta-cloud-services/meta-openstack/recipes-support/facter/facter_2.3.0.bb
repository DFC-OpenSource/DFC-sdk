SUMMARY = "Facter gathers basic facts about nodes (systems)"
HOMEPAGE = "http://puppetlabs.com/facter"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=ce69a88023d6f6ab282865ddef9f1e41"

SRC_URI = " \
    http://downloads.puppetlabs.com/facter/facter-${PV}.tar.gz \
    file://add_facter_gemspec.patch \
"
SRC_URI[md5sum] = "7bb6dbeaef86cd79300b4723c06932bc"
SRC_URI[sha256sum] = "a91ea915b276172e002a8670684e5c6be7df1dfdd55db6937d27fffad70c5e51"

inherit ruby

DEPENDS += " \
        ruby \
"

RUBY_INSTALL_GEMS = "facter-${PV}.gem"
