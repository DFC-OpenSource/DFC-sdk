inherit meta toolchain-scripts

# Wildcards specifying complementary packages to install for every package that has been explicitly
# installed into the rootfs
COMPLEMENTARY_GLOB[dev-pkgs] = '*-dev'
COMPLEMENTARY_GLOB[staticdev-pkgs] = '*-staticdev'
COMPLEMENTARY_GLOB[doc-pkgs] = '*-doc'
COMPLEMENTARY_GLOB[dbg-pkgs] = '*-dbg'
COMPLEMENTARY_GLOB[ptest-pkgs] = '*-ptest'

def complementary_globs(featurevar, d):
    all_globs = d.getVarFlags('COMPLEMENTARY_GLOB')
    globs = []
    features = set((d.getVar(featurevar, True) or '').split())
    for name, glob in all_globs.items():
        if name in features:
            globs.append(glob)
    return ' '.join(globs)

SDKIMAGE_FEATURES ??= "dev-pkgs dbg-pkgs"
SDKIMAGE_INSTALL_COMPLEMENTARY = '${@complementary_globs("SDKIMAGE_FEATURES", d)}'

inherit rootfs_${IMAGE_PKGTYPE}

SDK_DIR = "${WORKDIR}/sdk"
SDK_OUTPUT = "${SDK_DIR}/image"
SDK_DEPLOY = "${DEPLOY_DIR}/sdk"

B_task-populate-sdk = "${SDK_DIR}"

SDKTARGETSYSROOT = "${SDKPATH}/sysroots/${REAL_MULTIMACH_TARGET_SYS}"

TOOLCHAIN_HOST_TASK ?= "nativesdk-packagegroup-sdk-host packagegroup-cross-canadian-${MACHINE}"
TOOLCHAIN_HOST_TASK_ATTEMPTONLY ?= ""
TOOLCHAIN_TARGET_TASK ?= " \
    ${@multilib_pkg_extend(d, 'packagegroup-core-standalone-sdk-target')} \
    ${@multilib_pkg_extend(d, 'packagegroup-core-standalone-sdk-target-dbg')} \
    "
TOOLCHAIN_TARGET_TASK_ATTEMPTONLY ?= ""
TOOLCHAIN_OUTPUTNAME ?= "${SDK_NAME}-toolchain-${SDK_VERSION}"

SDK_RDEPENDS = "${TOOLCHAIN_TARGET_TASK} ${TOOLCHAIN_HOST_TASK}"
SDK_DEPENDS = "virtual/fakeroot-native"

# We want the MULTIARCH_TARGET_SYS to point to the TUNE_PKGARCH, not PACKAGE_ARCH as it
# could be set to the MACHINE_ARCH
REAL_MULTIMACH_TARGET_SYS = "${TUNE_PKGARCH}${TARGET_VENDOR}-${TARGET_OS}"

PID = "${@os.getpid()}"

EXCLUDE_FROM_WORLD = "1"

SDK_PACKAGING_FUNC ?= "create_shar"
SDK_POST_INSTALL_COMMAND ?= ""
SDK_RELOCATE_AFTER_INSTALL ?= "1"

SDK_MANIFEST = "${SDK_DEPLOY}/${TOOLCHAIN_OUTPUTNAME}.manifest"
python write_target_sdk_manifest () {
    from oe.sdk import sdk_list_installed_packages
    sdkmanifestdir = os.path.dirname(d.getVar("SDK_MANIFEST", True))
    if not os.path.exists(sdkmanifestdir):
        bb.utils.mkdirhier(sdkmanifestdir)
    with open(d.getVar('SDK_MANIFEST', True), 'w') as output:
        output.write(sdk_list_installed_packages(d, True, 'ver'))
}

POPULATE_SDK_POST_TARGET_COMMAND_append = " write_target_sdk_manifest ; "

fakeroot python do_populate_sdk() {
    from oe.sdk import populate_sdk
    from oe.manifest import create_manifest, Manifest

    pn = d.getVar('PN', True)
    runtime_mapping_rename("TOOLCHAIN_TARGET_TASK", pn, d)
    runtime_mapping_rename("TOOLCHAIN_TARGET_TASK_ATTEMPTONLY", pn, d)

    ld = bb.data.createCopy(d)
    ld.setVar("PKGDATA_DIR", "${STAGING_DIR}/${SDK_ARCH}-${SDKPKGSUFFIX}${SDK_VENDOR}-${SDK_OS}/pkgdata")
    runtime_mapping_rename("TOOLCHAIN_HOST_TASK", pn, ld)
    runtime_mapping_rename("TOOLCHAIN_HOST_TASK_ATTEMPTONLY", pn, ld)
    d.setVar("TOOLCHAIN_HOST_TASK", ld.getVar("TOOLCHAIN_HOST_TASK", True))
    d.setVar("TOOLCHAIN_HOST_TASK_ATTEMPTONLY", ld.getVar("TOOLCHAIN_HOST_TASK_ATTEMPTONLY", True))
    
    # create target/host SDK manifests
    create_manifest(d, manifest_dir=d.getVar('SDK_DIR', True),
                    manifest_type=Manifest.MANIFEST_TYPE_SDK_HOST)
    create_manifest(d, manifest_dir=d.getVar('SDK_DIR', True),
                    manifest_type=Manifest.MANIFEST_TYPE_SDK_TARGET)

    populate_sdk(d)

    # Process DEFAULTTUNE
    bb.build.exec_func("create_sdk_files", d)

    bb.build.exec_func("tar_sdk", d)

    sdk_packaging_func = d.getVar("SDK_PACKAGING_FUNC", True) or ""
    if sdk_packaging_func.strip():
        bb.build.exec_func(d.getVar("SDK_PACKAGING_FUNC", True), d)
}

fakeroot create_sdk_files() {
	cp ${COREBASE}/scripts/relocate_sdk.py ${SDK_OUTPUT}/${SDKPATH}/

	# Replace the ##DEFAULT_INSTALL_DIR## with the correct pattern.
	# Escape special characters like '+' and '.' in the SDKPATH
	escaped_sdkpath=$(echo ${SDKPATH} |sed -e "s:[\+\.]:\\\\\\\\\0:g")
	sed -i -e "s:##DEFAULT_INSTALL_DIR##:$escaped_sdkpath:" ${SDK_OUTPUT}/${SDKPATH}/relocate_sdk.py
}

SDKTAROPTS = "--owner=root --group=root -j"

fakeroot tar_sdk() {
	# Package it up
	mkdir -p ${SDK_DEPLOY}
	cd ${SDK_OUTPUT}/${SDKPATH}
	tar ${SDKTAROPTS} -c --file=${SDK_DEPLOY}/${TOOLCHAIN_OUTPUTNAME}.tar.bz2 .
}

fakeroot create_shar() {
	# copy in the template shar extractor script
	cp ${COREBASE}/meta/files/toolchain-shar-extract.sh ${SDK_DEPLOY}/${TOOLCHAIN_OUTPUTNAME}.sh

	rm -f ${T}/post_install_command

	if [ ${SDK_RELOCATE_AFTER_INSTALL} -eq 1 ] ; then
		cp ${COREBASE}/meta/files/toolchain-shar-relocate.sh ${T}/post_install_command
	fi
	cat << "EOF" >> ${T}/post_install_command
${SDK_POST_INSTALL_COMMAND}
EOF
	sed -i -e '/@SDK_POST_INSTALL_COMMAND@/r ${T}/post_install_command' ${SDK_DEPLOY}/${TOOLCHAIN_OUTPUTNAME}.sh

	# substitute variables
	sed -i -e 's#@SDK_ARCH@#${SDK_ARCH}#g' \
		-e 's#@SDKPATH@#${SDKPATH}#g' \
		-e 's#@OLDEST_KERNEL@#${OLDEST_KERNEL}#g' \
		-e 's#@REAL_MULTIMACH_TARGET_SYS@#${REAL_MULTIMACH_TARGET_SYS}#g' \
		-e '/@SDK_POST_INSTALL_COMMAND@/d' \
		${SDK_DEPLOY}/${TOOLCHAIN_OUTPUTNAME}.sh

	# add execution permission
	chmod +x ${SDK_DEPLOY}/${TOOLCHAIN_OUTPUTNAME}.sh

	# append the SDK tarball
	cat ${SDK_DEPLOY}/${TOOLCHAIN_OUTPUTNAME}.tar.bz2 >> ${SDK_DEPLOY}/${TOOLCHAIN_OUTPUTNAME}.sh

	# delete the old tarball, we don't need it anymore
	rm ${SDK_DEPLOY}/${TOOLCHAIN_OUTPUTNAME}.tar.bz2
}

populate_sdk_log_check() {
	for target in $*
	do
		lf_path="`dirname ${BB_LOGFILE}`/log.do_$target.${PID}"

		echo "log_check: Using $lf_path as logfile"

		if test -e "$lf_path"
		then
			${IMAGE_PKGTYPE}_log_check $target $lf_path
		else
			echo "Cannot find logfile [$lf_path]"
		fi
		echo "Logfile is clean"
	done
}

do_populate_sdk[dirs] = "${TOPDIR}"
do_populate_sdk[depends] += "${@' '.join([x + ':do_populate_sysroot' for x in d.getVar('SDK_DEPENDS', True).split()])}  ${@d.getVarFlag('do_rootfs', 'depends', False)}"
do_populate_sdk[rdepends] = "${@' '.join([x + ':do_populate_sysroot' for x in d.getVar('SDK_RDEPENDS', True).split()])}"
do_populate_sdk[recrdeptask] += "do_packagedata do_package_write_rpm do_package_write_ipk do_package_write_deb"
addtask populate_sdk
