SUMMARY = "Open Fabrics Enterprise Distribution (OFED)"
HOMEPAGE = "http://openfabrics.org"
SECTION = "console/utils"
LICENSE = "BSD-3-Clause"
LIC_FILES_CHKSUM = "file://${WORKDIR}/OFED-${PV}/LICENSE;md5=2c688cb8b2a7e5fa0c18cc5737cd0cee"


#SRC_URI = "http://downloads.openfabrics.org/OFED/archive/ofed-1.5.2/OFED-1.5.2.tgz"
SRC_URI = "file://OFED-1.5.2.tgz\
	   file://install_pl.patch\
	   file://ofed_beta_config.conf\
	   file://librxe.zip\
	   file://patches.zip\
	   file://patch_srpms_script_generator.sh"


SRC_URI[md5sum] = "29fb2ae175dc912079c34b49f9de90c3"
SRC_URI[sha256sum] = "60aca976b89cca2ce8b57a2b925d9731096c21980d60fa954927fb3c835d5f51"

RDEPENDS = "tcsh"\

do_patch() {

	echo "present working dir is "
	echo $PWD
	cd OFED-1.5.2
	patch -p0 < ../install_pl.patch
	cp  ../ofed_beta_config.conf .
	cp  ../librxe-0.9.0-1.src.rpm  SRPMS/
	cp ../patch_srpms_script_generator.sh .
	cp -r ../patches  .
	chmod +x patch_srpms_script_generator.sh
	./patch_srpms_script_generator.sh
	./patch_script.sh
	cd -
}

do_install() {


	echo "Removing the temporary rpmdb database directory"
	rm -rf ${WORKDIR}/ofed_tmp_rpmdb

	echo "starting ofed installation with max verbose level (-vvv)"

	${WORKDIR}/OFED-${PV}/install.pl -vvv --prefix ${WORKDIR}/mergeable_rootfs --builddir ${WORKDIR}/temp_builddir -c ${WORKDIR}/OFED-${PV}/ofed_beta_config.conf

	echo "removing static libraries (Merging should not give any issues with static librarires)"

	find  ${WORKDIR}/mergeable_rootfs  -name "*.a" | xargs rm
	
	echo "changing RPATH of binaries to /lib"

	cd ${WORKDIR}/mergeable_rootfs/bin
	find . | xargs file   | grep "ELF"  |  awk {'print "chrpath --replace /lib " $1 '} |  cut -d ':' -f1 > binaryfilelisting_for_changing_rpath.sh
	echo "Always exit the binaryfilelisting_for_changing_rpath.sh with exit status 0"
	echo "exit 0" >> binaryfilelisting_for_changing_rpath.sh
	chmod +x binaryfilelisting_for_changing_rpath.sh
	./binaryfilelisting_for_changing_rpath.sh
	echo "creating a shortcut for tcsh with name of csh, deleting shortcut incase already exists and recreating shortcut to avoid build failure"
	rm -rf csh
	ln -s /usr/bin/tcsh csh
	cd -

	
	cd ${WORKDIR}/mergeable_rootfs/lib
	#mv libcxgb3-rdmav2.so libcxgb3.so.1.0.0
	#mv libmlx4-rdmav2.so libmlx4.so.1.0.0
	#mv libmthca-rdmav2.so  libmthca.so.1.0.0
	#mv libnes-rdmav2.so libnes.so.1.0.0
	mv librxe-rdmav2.so librxe.so.1.0.0
	cd -

	echo "removing csh files from /etc/profile.d"
	rm -rf ${WORKDIR}/mergeable_rootfs/etc/profile.d/*.csh
	rm -rf ${WORKDIR}/mergeable_rootfs/include
	rm -rf ${WORKDIR}/mergeable_rootfs/doc
	rm -rf ${WORKDIR}/mergeable_rootfs/share
	
	echo "copying to ${D} so that installation is mergeable into rootfs"	
	cp -r  ${WORKDIR}/mergeable_rootfs/* ${D}/
}

INSANE_SKIP_${PN} += "file-rdeps useless-rpaths"
