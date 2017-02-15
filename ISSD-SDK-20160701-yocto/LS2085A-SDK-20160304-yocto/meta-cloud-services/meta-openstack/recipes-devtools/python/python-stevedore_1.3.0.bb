DESCRIPTION = "Manage dynamic plugins for Python applications"
HOMEPAGE = "https://github.com/dreamhost/stevedore"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=3b83ef96387f14655fc854ddc3c6bd57"

SRC_URI[md5sum] = "e9ed2a1cf91ee76fd9045f902fa58089"
SRC_URI[sha256sum] = "beab2b7f91966d259796392c39ed6f260b32851861561dd9f3b9be2fd0c426a5"

inherit pypi rmargparse

DEPENDS += " \
        python-pip \
        python-pbr \
        "

RDEPENDS_${PN} += " \
        python-pbr \
        "
