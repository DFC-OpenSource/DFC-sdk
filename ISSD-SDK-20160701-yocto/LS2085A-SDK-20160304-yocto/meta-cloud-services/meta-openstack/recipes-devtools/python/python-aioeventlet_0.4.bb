DESCRIPTION = "asyncio event loop scheduling callbacks in eventlet"
HOMEPAGE = "https://pypi.python.org/pypi/aioeventlet"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://COPYING;md5=8f7bb094c7232b058c7e9f2e431f389c"

SRC_URI[md5sum] = "678ea30265ae0326bddc767f80efd144"
SRC_URI[sha256sum] = "fe78c2b227ce077b1581e2ae2c071f351111d0878ec1b0216435f6a898df79a6"

inherit pypi

RDEPENDS_${PN} += " \
    python-trollius \
    "
