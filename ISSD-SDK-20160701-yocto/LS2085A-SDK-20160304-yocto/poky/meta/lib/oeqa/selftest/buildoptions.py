import unittest
import os
import logging
import re

from oeqa.selftest.base import oeSelfTest
from oeqa.selftest.buildhistory import BuildhistoryBase
from oeqa.utils.commands import runCmd, bitbake, get_bb_var
import oeqa.utils.ftools as ftools
from oeqa.utils.decorators import testcase

class ImageOptionsTests(oeSelfTest):

    @testcase(761)
    def test_incremental_image_generation(self):
        image_pkgtype = get_bb_var("IMAGE_PKGTYPE")
        if image_pkgtype != 'rpm':
            self.skipTest('Not using RPM as main package format')
        bitbake("-c cleanall core-image-minimal")
        self.write_config('INC_RPM_IMAGE_GEN = "1"')
        self.append_config('IMAGE_FEATURES += "ssh-server-openssh"')
        bitbake("core-image-minimal")
        log_data_file = os.path.join(get_bb_var("WORKDIR", "core-image-minimal"), "temp/log.do_rootfs")
        log_data_created = ftools.read_file(log_data_file)
        incremental_created = re.search("NOTE: load old install solution for incremental install\nNOTE: old install solution not exist\nNOTE: creating new install solution for incremental install(\n.*)*NOTE: Installing the following packages:.*packagegroup-core-ssh-openssh", log_data_created)
        self.remove_config('IMAGE_FEATURES += "ssh-server-openssh"')
        self.assertTrue(incremental_created, msg = "Match failed in:\n%s" % log_data_created)
        bitbake("core-image-minimal")
        log_data_removed = ftools.read_file(log_data_file)
        incremental_removed = re.search("NOTE: load old install solution for incremental install\nNOTE: creating new install solution for incremental install(\n.*)*NOTE: incremental removed:.*openssh-sshd-.*", log_data_removed)
        self.assertTrue(incremental_removed, msg = "Match failed in:\n%s" % log_data_removed)

    @testcase(925)
    def test_rm_old_image(self):
        bitbake("core-image-minimal")
        deploydir = get_bb_var("DEPLOY_DIR_IMAGE", target="core-image-minimal")
        imagename = get_bb_var("IMAGE_LINK_NAME", target="core-image-minimal")
        deploydir_files = os.listdir(deploydir)
        track_original_files = []
        for image_file in deploydir_files:
            if imagename in image_file and os.path.islink(os.path.join(deploydir, image_file)):
                track_original_files.append(os.path.realpath(os.path.join(deploydir, image_file)))
        self.append_config("RM_OLD_IMAGE = \"1\"")
        bitbake("-C rootfs core-image-minimal")
        deploydir_files = os.listdir(deploydir)
        remaining_not_expected = [path for path in track_original_files if os.path.basename(path) in deploydir_files]
        self.assertFalse(remaining_not_expected, msg="\nThe following image files ware not removed: %s" % ', '.join(map(str, remaining_not_expected)))

    @testcase(286)
    def test_ccache_tool(self):
        bitbake("ccache-native")
        self.assertTrue(os.path.isfile(os.path.join(get_bb_var('STAGING_BINDIR_NATIVE', 'ccache-native'), "ccache")))
        self.write_config('INHERIT += "ccache"')
        bitbake("m4 -c cleansstate")
        bitbake("m4 -c compile")
        res = runCmd("grep ccache %s" % (os.path.join(get_bb_var("WORKDIR","m4"),"temp/log.do_compile")), ignore_status=True)
        self.assertEqual(0, res.status, msg="No match for ccache in m4 log.do_compile")
        bitbake("ccache-native -ccleansstate")


class DiskMonTest(oeSelfTest):

    @testcase(277)
    def test_stoptask_behavior(self):
        self.write_config('BB_DISKMON_DIRS = "STOPTASKS,${TMPDIR},100000G,100K"')
        res = bitbake("m4", ignore_status = True)
        self.assertTrue('ERROR: No new tasks can be executed since the disk space monitor action is "STOPTASKS"!' in res.output)
        self.assertEqual(res.status, 1)
        self.write_config('BB_DISKMON_DIRS = "ABORT,${TMPDIR},100000G,100K"')
        res = bitbake("m4", ignore_status = True)
        self.assertTrue('ERROR: Immediately abort since the disk space monitor action is "ABORT"!' in res.output)
        self.assertEqual(res.status, 1)
        self.write_config('BB_DISKMON_DIRS = "WARN,${TMPDIR},100000G,100K"')
        res = bitbake("m4")
        self.assertTrue('WARNING: The free space' in res.output)

class SanityOptionsTest(oeSelfTest):

    @testcase(927)
    def test_options_warnqa_errorqa_switch(self):
        bitbake("xcursor-transparent-theme -ccleansstate")

        if "packages-list" not in get_bb_var("ERROR_QA"):
            self.write_config("ERROR_QA_append = \" packages-list\"")

        self.write_recipeinc('xcursor-transparent-theme', 'PACKAGES += \"${PN}-dbg\"')
        res = bitbake("xcursor-transparent-theme", ignore_status=True)
        self.delete_recipeinc('xcursor-transparent-theme')
        self.assertTrue("ERROR: QA Issue: xcursor-transparent-theme-dbg is listed in PACKAGES multiple times, this leads to packaging errors." in res.output, msg=res.output)
        self.assertEqual(res.status, 1)
        self.write_recipeinc('xcursor-transparent-theme', 'PACKAGES += \"${PN}-dbg\"')
        self.append_config('ERROR_QA_remove = "packages-list"')
        self.append_config('WARN_QA_append = " packages-list"')
        bitbake("xcursor-transparent-theme -ccleansstate")
        res = bitbake("xcursor-transparent-theme")
        self.delete_recipeinc('xcursor-transparent-theme')
        self.assertTrue("WARNING: QA Issue: xcursor-transparent-theme-dbg is listed in PACKAGES multiple times, this leads to packaging errors." in res.output, msg=res.output)

    @testcase(278)
    def test_sanity_userspace_dependency(self):
        self.append_config('WARN_QA_append = " unsafe-references-in-binaries unsafe-references-in-scripts"')
        bitbake("-ccleansstate gzip nfs-utils")
        res = bitbake("gzip nfs-utils")
        self.assertTrue("WARNING: QA Issue: gzip" in res.output)
        self.assertTrue("WARNING: QA Issue: nfs-utils" in res.output)

class BuildhistoryTests(BuildhistoryBase):

    @testcase(293)
    def test_buildhistory_basic(self):
        self.run_buildhistory_operation('xcursor-transparent-theme')
        self.assertTrue(os.path.isdir(get_bb_var('BUILDHISTORY_DIR')))

    @testcase(294)
    def test_buildhistory_buildtime_pr_backwards(self):
        self.add_command_to_tearDown('cleanup-workdir')
        target = 'xcursor-transparent-theme'
        error = "ERROR: QA Issue: Package version for package %s went backwards which would break package feeds from (.*-r1 to .*-r0)" % target
        self.run_buildhistory_operation(target, target_config="PR = \"r1\"", change_bh_location=True)
        self.run_buildhistory_operation(target, target_config="PR = \"r0\"", change_bh_location=False, expect_error=True, error_regex=error)






