DESCRIPTION = "netkit-rsh includes the rsh daemon and client."
SECTION = "networking"
HOMEPAGE="ftp://ftp.uk.linux.org/pub/linux/Networking/netkit"
LICENSE = "BSD-4-Clause"
DEPENDS = "xinetd libgcrypt"

LIC_FILES_CHKSUM = "file://rsh/rsh.c;beginline=2;endline=3;md5=25737e9d21d9df251dd26b7dcbd8ee29"

SRC_URI = "${DEBIAN_MIRROR}/main/n/netkit-rsh/netkit-rsh_${PV}.orig.tar.gz;name=archive \
            ${DEBIAN_MIRROR}/main/n/netkit-rsh/netkit-rsh_${PV}-15.diff.gz;name=patch15 \
            file://rsh-redone_link_order_file.patch \
            file://no_pam_build_fix.patch \
            file://rexec.xinetd.netkit \
            file://rlogin.xinetd.netkit \
            file://rsh.xinetd.netkit \
"

SRC_URI[archive.md5sum] = "65f5f28e2fe22d9ad8b17bb9a10df096"
SRC_URI[archive.sha256sum] = "edcac7fa18015f0bc04e573f3f54ae3b638d71335df1ad7dae692779914ad669"
SRC_URI[patch15.md5sum] = "655efc0d541b03ca5de0ae506c805ea3"
SRC_URI[patch15.sha256sum] = "2bc071c438e8b0ed42a0bd2db2d8b681b27a1e9b1798694d9874733293bc2aa9"

inherit pkgconfig

CFLAGS += " -D_GNU_SOURCE -Wno-deprecated-declarations"
LDFLAGS += " -L${STAGING_LIBDIR} -lutil -lcrypt"

PACKAGECONFIG ??= ""
PACKAGECONFIG += " ${@bb.utils.contains("DISTRO_FEATURES", "pam", "pam", "", d)}"
PACKAGECONFIG[pam] = " , --without-pam, libpam, libpam"

do_configure () {
    ./configure --prefix=${prefix}
    echo "INSTALLROOT=${D}" > MCONFIG

    if [ "${@base_contains('PACKAGECONFIG', 'pam', 'pam', '', d)}" != "" ]; then
        echo "USE_PAM=1" >> MCONFIG
    fi

    # didn't want to patch these next changes
    sed -i 's/netkit-//' ${S}/rsh/pathnames.h
    sed -i 's/netkit-//' ${S}/rcp/pathnames.h
}

do_install () {
    install -d ${D}${bindir}
    install -d ${D}${sbindir}
    install -d ${D}${mandir}/man1
    install -d ${D}${mandir}/man8
    install -d ${D}${sysconfdir}/xinetd.d

    oe_runmake 'INSTALLROOT=${D}' 'BINMODE=0755' \
    'DAEMONMODE=0755' 'MANMODE=0644' \
    'SUIDMODE=4755' \
    'BINDIR=${bindir}' 'SBINDIR=${sbindir}' \
    'MANDIR=${mandir}' install

    if [ "${@base_contains('PACKAGECONFIG', 'pam', 'pam', '', d)}" != "" ]; then
        install -d ${D}${sysconfdir}/pam.d
        install -m 0644 debian/hosts.equiv ${D}/${sysconfdir}
        install -m 0644 debian/pam.d/rexec ${D}/${sysconfdir}/pam.d
        install -m 0644 debian/pam.d/rlogin ${D}/${sysconfdir}/pam.d
        install -m 0644 debian/pam.d/rsh ${D}/${sysconfdir}/pam.d
    fi
    cp ${WORKDIR}/rexec.xinetd.netkit  ${D}/${sysconfdir}/xinetd.d/rexec
    cp ${WORKDIR}/rlogin.xinetd.netkit  ${D}/${sysconfdir}/xinetd.d/rlogin
    cp ${WORKDIR}/rsh.xinetd.netkit  ${D}/${sysconfdir}/xinetd.d/rsh
}

PACKAGES = "${PN}-client ${PN}-server ${PN}-doc ${BPN}-dbg"

FILES_${PN}-client = "${bindir}/*"
FILES_${PN}-server = "${sbindir}/* ${sysconfdir}"
FILES_${PN}-doc = "${mandir}"
FILES_${PN}-dbg = "${prefix}/src/debug \
                   ${sbindir}/.debug ${bindir}/.debug"

ALTERNATIVE_PRIORITY = "80"
ALTERNATIVE_${PN}-client = "rcp rexec rlogin rsh"
ALTERNATIVE_${PN}-server = "rshd rexecd rlogind"
ALTERNATIVE_LINK_NAME[server] = "${bindir}/rshd"
ALTERNATIVE_TARGET[rshd] = "${sbindir}/in.rshd"
ALTERNATIVE_LINK_NAME[rexecd] = "${bindir}/rexecd"
ALTERNATIVE_TARGET[rexecd] = "${sbindir}/in.rexecd"
ALTERNATIVE_LINK_NAME[rlogind] = "${bindir}/rlogind"
ALTERNATIVE_TARGET[rlogind] = "${sbindir}/in.rlogind"

RCONFLICTS_${PN}-client += "inetutils-rshd"
RPROVIDES_${PN}-rshd = "rshd"

RDEPENDS_${PN} = "xinetd"
