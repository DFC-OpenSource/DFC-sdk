 #!/bin/bash

rm -rf patch_script.sh
echo "#!/bin/bash"  >> patch_script.sh

patch_and_rebuild_srpms() {

entry=$PWD
echo "rpm -iv  --nodeps --define '_topdir "$8"' SRPMS/$2 --dbpath $entry/RPM_DB_TEMP" >> patch_script.sh
echo "cd $8/SOURCES" >> patch_script.sh
echo "tar $4 $3" >> patch_script.sh
echo "cd $8" >> patch_script.sh
echo "patch -p1 < $6/$7" >> patch_script.sh
echo "cd $8/SOURCES" >> patch_script.sh
echo "tar $5 $3 $9" >> patch_script.sh
echo "rpmbuild -bs  --nodeps --define '_topdir "$8"' $8/SPECS/${10} --dbpath $entry/RPM_DB_TEMP" >> patch_script.sh
echo "cp $8/SRPMS/$2 $1" >>  patch_script.sh
echo  "cd $entry" >> patch_script.sh

}

echo "rm -rf $PWD/RPM_DB_TEMP" >> patch_script.sh


#		 [1]                  [2]         [3]          [4]             [5]            [6]             [7]         [8]                       [9]             [10]
#function_name  ORIGNAL_SRPMS_PATH SRPM_NAME TARBALL_NAME INFLATE_OPTIONS DEFLATE_OPTIONS PATCH_DIRECTORY PATCH_NAME SRPMS_WORKING_DIR  COMPRESS_DIRECTORY_NAME SPEC_FILE_NAME
#1 ofed-scripts
#patch_and_rebuild_srpms $PWD/"SRPMS" "ofed-scripts-1.5.2-OFED.1.5.2..src.rpm"  "ofed-scripts-1.5.2.tar.gz" "-xvpf" "-zcvf" $PWD/"patches"  "ofed-scripts.patch" $PWD/"SRPMS_WORKING_DIR"  "ofed-scripts-1.5.2" "ofed-scripts.spec"
patch_and_rebuild_srpms $PWD/"SRPMS" "ofed-scripts-1.5.2-OFED.1.5.2..src.rpm"  "ofed-scripts-1.5.2.tar.gz" "-xvpf" "-zcvf" $PWD/"patches"  "ofed-scripts_july_20.patch" $PWD/"SRPMS_WORKING_DIR"  "ofed-scripts-1.5.2" "ofed-scripts.spec"

#2 compat-dapl
patch_and_rebuild_srpms $PWD/"SRPMS" "compat-dapl-1.2.19-1.src.rpm"  "compat-dapl-1.2.19.tar.gz" "-xvpf" "-zcvf" $PWD/"patches"  "compat-dapl.patch" $PWD/"SRPMS_WORKING_DIR"  "compat-dapl-1.2.19" "dapl.spec"

#3 dapl
patch_and_rebuild_srpms $PWD/"SRPMS" "dapl-2.0.30-1.src.rpm"  "dapl-2.0.30.tar.gz" "-xvpf" "-zcvf" $PWD/"patches"  "dapl.patch" $PWD/"SRPMS_WORKING_DIR"  "dapl-2.0.30" "dapl.spec"

#4 infiniband-diags
patch_and_rebuild_srpms $PWD/"SRPMS" "infiniband-diags-1.5.7-1.src.rpm"  "infiniband-diags-1.5.7.tar.gz" "-xvpf" "-zcvf" $PWD/"patches"  "infiniband_diags.patch" $PWD/"SRPMS_WORKING_DIR"  "infiniband-diags-1.5.7" "infiniband-diags.spec"

#5 libibverbs
patch_and_rebuild_srpms $PWD/"SRPMS" "libibverbs-1.1.4-0.14.gb6c138b.src.rpm"  "libibverbs-1.1.4-0.14.gb6c138b.tar.gz" "-xvpf" "-zcvf" $PWD/"patches"  "libibverbs.patch" $PWD/"SRPMS_WORKING_DIR"  "libibverbs-1.1.4" "libibverbs.spec"

#6 librdmacm
patch_and_rebuild_srpms $PWD/"SRPMS" "librdmacm-1.0.13-1.src.rpm"  "librdmacm-1.0.13.tar.gz" "-xvpf" "-zcvf" $PWD/"patches"  "librdmacm.patch" $PWD/"SRPMS_WORKING_DIR"  "librdmacm-1.0.13" "librdmacm.spec"

#7 mpi-selector
#patch_and_rebuild_srpms $PWD/"SRPMS" "mpi-selector-1.0.3-1.src.rpm"  "mpi-selector-1.0.3.tar.gz" "-xvpf" "-zcvf" $PWD/"patches"  "mpi-selector.patch" $PWD/"SRPMS_WORKING_DIR"  "mpi-selector-1.0.3" "mpi-selector.spec"
patch_and_rebuild_srpms $PWD/"SRPMS" "mpi-selector-1.0.3-1.src.rpm"  "mpi-selector-1.0.3.tar.gz" "-xvpf" "-zcvf" $PWD/"patches"  "mpi-selector_july_20.patch" $PWD/"SRPMS_WORKING_DIR"  "mpi-selector-1.0.3" "mpi-selector.spec"

#8 openmpi
patch_and_rebuild_srpms $PWD/"SRPMS" "openmpi-1.4.2-1.src.rpm"  "openmpi-1.4.2.tar.bz2" "-xvpf" "-zjvf" $PWD/"patches"  "openmpi.patch" $PWD/"SRPMS_WORKING_DIR"  "openmpi-1.4.2" "openmpi-1.4.2.spec"

#9 perftest
patch_and_rebuild_srpms $PWD/"SRPMS" "perftest-1.3.0-0.28.gca1c30b.src.rpm"  "perftest-1.3.0-0.28.gca1c30b.tar.gz" "-xvpf" "-zcvf" $PWD/"patches"  "perftest.patch" $PWD/"SRPMS_WORKING_DIR"  "perftest-1.3.0" "perftest.spec"

#10 qperf
patch_and_rebuild_srpms $PWD/"SRPMS" "qperf-0.4.6-0.1.gb81434e.src.rpm"  "qperf-0.4.6-0.1.gb81434e.tar.gz" "-xvpf" "-zcvf" $PWD/"patches"  "qperf.patch" $PWD/"SRPMS_WORKING_DIR"  "qperf-0.4.6" "qperf.spec"


#permissions for generated script
chmod +x patch_script.sh


