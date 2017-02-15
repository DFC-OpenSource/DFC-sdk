DESCRIPTION = "Format agnostic tabular data library (XLS, JSON, YAML, CSV)"
SECTION = "devel/python"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=d26f98effd307261a2e0ddcce08e07b7"

SRC_URI[md5sum] = "41fb8a731f6ef54812045ef9bf2909c6"
SRC_URI[sha256sum] = "41c2dad7f491f5557e22783a9af81bed62f7b6fb0d7afd4c2ee301f3eb428c93"

inherit pypi

DEPENDS += " \
    python-pip \
    "

RDEPENDS_${PN} += " \
    python-pip \
    python-pyyaml \
    "
