SUMMARY = "Dummy package which ensures perl is excluded from buildtools"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

inherit nativesdk

# Put it somewhere separate to ensure it's never used except when we want it
PACKAGE_ARCH = "buildtools-dummy-${SDKPKGSUFFIX}"

PERLPACKAGES = "nativesdk-perl \
                nativesdk-perl-module-file-path"

ALLOW_EMPTY_${PN} = "1"

python populate_packages_prepend() {
    d.appendVar('RPROVIDES_${PN}', '${PERLPACKAGES}')
    d.appendVar('RCONFLICTS_${PN}', '${PERLPACKAGES}')
}

