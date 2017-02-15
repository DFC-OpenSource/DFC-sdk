DESCRIPTION = "Dummy packagegroup to provide virtual/monitoring"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/GPL-2.0;md5=801f80980d171dd6425610833a22dbe6"

inherit packagegroup

SRCNAME = "packagegroup-monitoring"

PROVIDES = "virtual/monitoring"

PACKAGES = "\
	${SRCNAME}-core \
	${SRCNAME}-agent \
"

RDEPENDS_${SRCNAME}-core += "\
"

RDEPENDS_${SRCNAME}-agent += "\
"
