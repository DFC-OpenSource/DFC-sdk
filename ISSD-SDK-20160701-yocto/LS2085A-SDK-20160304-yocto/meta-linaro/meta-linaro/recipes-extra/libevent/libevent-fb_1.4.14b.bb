require recipes-support/libevent/libevent_2.0.21.bb

PROVIDES = "libevent"

INC_PR = "1"

SRC_URI = "http://github.com/downloads/libevent/libevent/libevent-${PV}-stable.tar.gz;name=archive \
           http://libevent.org/LICENSE.txt;name=license \
           file://libevent-1.4.14.fb-changes.diff"

SRC_URI[archive.md5sum] = "a00e037e4d3f9e4fe9893e8a2d27918c"
SRC_URI[archive.sha256sum] = "afa61b476a222ba43fc7cca2d24849ab0bbd940124400cb699915d3c60e46301"

SRC_URI[license.md5sum] = "412e611443304db6a338ab32728ae297"
SRC_URI[license.sha256sum] = "55739d5492273a7058c66b682012330a84c34eaa666f5c7030b0312573235270"


S = "${WORKDIR}/libevent-${PV}-stable/"

LIC_FILES_CHKSUM = "file://${WORKDIR}/LICENSE.txt;md5=412e611443304db6a338ab32728ae297"
