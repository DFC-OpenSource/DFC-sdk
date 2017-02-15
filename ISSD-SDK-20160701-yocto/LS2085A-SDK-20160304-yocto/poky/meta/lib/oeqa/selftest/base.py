# Copyright (c) 2013 Intel Corporation
#
# Released under the MIT license (see COPYING.MIT)


# DESCRIPTION
# Base class inherited by test classes in meta/lib/selftest

import unittest
import os
import sys
import shutil
import logging
import errno

import oeqa.utils.ftools as ftools
from oeqa.utils.commands import runCmd, bitbake, get_bb_var, get_test_layer
from oeqa.utils.decorators import LogResults

@LogResults
class oeSelfTest(unittest.TestCase):

    log = logging.getLogger("selftest.base")
    longMessage = True

    def __init__(self, methodName="runTest"):
        self.builddir = os.environ.get("BUILDDIR")
        self.localconf_path = os.path.join(self.builddir, "conf/local.conf")
        self.testinc_path = os.path.join(self.builddir, "conf/selftest.inc")
        self.testlayer_path = oeSelfTest.testlayer_path
        self._extra_tear_down_commands = []
        self._track_for_cleanup = []
        super(oeSelfTest, self).__init__(methodName)

    def setUp(self):
        os.chdir(self.builddir)
        # we don't know what the previous test left around in config or inc files
        # if it failed so we need a fresh start
        try:
            os.remove(self.testinc_path)
        except OSError as e:
            if e.errno != errno.ENOENT:
                raise
        for root, _, files in os.walk(self.testlayer_path):
            for f in files:
                if f == 'test_recipe.inc':
                    os.remove(os.path.join(root, f))
        # tests might need their own setup
        # but if they overwrite this one they have to call
        # super each time, so let's give them an alternative
        self.setUpLocal()

    def setUpLocal(self):
        pass

    def tearDown(self):
        if self._extra_tear_down_commands:
            failed_extra_commands = []
            for command in self._extra_tear_down_commands:
                result = runCmd(command, ignore_status=True)
                if not result.status ==  0:
                    failed_extra_commands.append(command)
            if failed_extra_commands:
                self.log.warning("tearDown commands have failed: %s" % ', '.join(map(str, failed_extra_commands)))
                self.log.debug("Trying to move on.")
            self._extra_tear_down_commands = []

        if self._track_for_cleanup:
            for path in self._track_for_cleanup:
                if os.path.isdir(path):
                    shutil.rmtree(path)
                if os.path.isfile(path):
                    os.remove(path)
            self._track_for_cleanup = []

        self.tearDownLocal()

    def tearDownLocal(self):
        pass

    # add test specific commands to the tearDown method.
    def add_command_to_tearDown(self, command):
        self.log.debug("Adding command '%s' to tearDown for this test." % command)
        self._extra_tear_down_commands.append(command)
    # add test specific files or directories to be removed in the tearDown method
    def track_for_cleanup(self, path):
        self.log.debug("Adding path '%s' to be cleaned up when test is over" % path)
        self._track_for_cleanup.append(path)

    # write to <builddir>/conf/selftest.inc
    def write_config(self, data):
        self.log.debug("Writing to: %s\n%s\n" % (self.testinc_path, data))
        ftools.write_file(self.testinc_path, data)

    # append to <builddir>/conf/selftest.inc
    def append_config(self, data):
        self.log.debug("Appending to: %s\n%s\n" % (self.testinc_path, data))
        ftools.append_file(self.testinc_path, data)

    # remove data from <builddir>/conf/selftest.inc
    def remove_config(self, data):
        self.log.debug("Removing from: %s\n\%s\n" % (self.testinc_path, data))
        ftools.remove_from_file(self.testinc_path, data)

    # write to meta-sefltest/recipes-test/<recipe>/test_recipe.inc
    def write_recipeinc(self, recipe, data):
        inc_file = os.path.join(self.testlayer_path, 'recipes-test', recipe, 'test_recipe.inc')
        self.log.debug("Writing to: %s\n%s\n" % (inc_file, data))
        ftools.write_file(inc_file, data)

    # append data to meta-sefltest/recipes-test/<recipe>/test_recipe.inc
    def append_recipeinc(self, recipe, data):
        inc_file = os.path.join(self.testlayer_path, 'recipes-test', recipe, 'test_recipe.inc')
        self.log.debug("Appending to: %s\n%s\n" % (inc_file, data))
        ftools.append_file(inc_file, data)

    # remove data from meta-sefltest/recipes-test/<recipe>/test_recipe.inc
    def remove_recipeinc(self, recipe, data):
        inc_file = os.path.join(self.testlayer_path, 'recipes-test', recipe, 'test_recipe.inc')
        self.log.debug("Removing from: %s\n%s\n" % (inc_file, data))
        ftools.remove_from_file(inc_file, data)

    # delete meta-sefltest/recipes-test/<recipe>/test_recipe.inc file
    def delete_recipeinc(self, recipe):
        inc_file = os.path.join(self.testlayer_path, 'recipes-test', recipe, 'test_recipe.inc')
        self.log.debug("Deleting file: %s" % inc_file)
        try:
            os.remove(inc_file)
        except OSError as e:
            if e.errno != errno.ENOENT:
                raise
