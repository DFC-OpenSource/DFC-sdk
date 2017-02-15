SUMMARY = "A simple pluggable Hierarchical Database"
HOMEPAGE = "https://projects.puppetlabs.com/projects/hiera"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=8ac92c91fbec097f798223679c1a7491"

SRC_URI = " \
    https://downloads.puppetlabs.com/hiera/hiera-${PV}.tar.gz \
    file://add_hiera_gemspec.patch \
"
SRC_URI[md5sum] = "6abccc518edb55291a63129ef30888cd"
SRC_URI[sha256sum] = "d3ecbfedc7d8493fd00c7dd624efbb225705d289442fe7706cb81a3a7230e70e"


inherit ruby

DEPENDS += " \
        ruby \
"

RUBY_INSTALL_GEMS = "hiera-${PV}.gem"
