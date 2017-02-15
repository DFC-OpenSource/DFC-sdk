#
# ex:ts=4:sw=4:sts=4:et
# -*- tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-
#
# BitBake Toaster Implementation
#
# Copyright (C) 2014        Intel Corporation
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.


import os
import sys
import re
from django.db import transaction
from django.db.models import Q
from bldcontrol.models import BuildEnvironment, BRLayer, BRVariable, BRTarget, BRBitbake
import subprocess

from toastermain import settings

from bbcontroller import BuildEnvironmentController, ShellCmdException, BuildSetupException

import logging
logger = logging.getLogger("toaster")

from pprint import pprint, pformat

class LocalhostBEController(BuildEnvironmentController):
    """ Implementation of the BuildEnvironmentController for the localhost;
        this controller manages the default build directory,
        the server setup and system start and stop for the localhost-type build environment

    """

    def __init__(self, be):
        super(LocalhostBEController, self).__init__(be)
        self.dburl = settings.getDATABASE_URL()
        self.pokydirname = None
        self.islayerset = False

    def _shellcmd(self, command, cwd = None):
        if cwd is None:
            cwd = self.be.sourcedir

        #logger.debug("lbc_shellcmmd: (%s) %s" % (cwd, command))
        p = subprocess.Popen(command, cwd = cwd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        (out,err) = p.communicate()
        p.wait()
        if p.returncode:
            if len(err) == 0:
                err = "command: %s \n%s" % (command, out)
            else:
                err = "command: %s \n%s" % (command, err)
            #logger.warn("localhostbecontroller: shellcmd error %s" % err)
            raise ShellCmdException(err)
        else:
            #logger.debug("localhostbecontroller: shellcmd success")
            return out

    def _createdirpath(self, path):
        from os.path import dirname as DN
        if path == "":
            raise Exception("Invalid path creation specified.")
        if not os.path.exists(DN(path)):
            self._createdirpath(DN(path))
        if not os.path.exists(path):
            os.mkdir(path, 0755)

    def _setupBE(self):
        assert self.pokydirname and os.path.exists(self.pokydirname)
        self._createdirpath(self.be.builddir)
        self._shellcmd("bash -c \"source %s/oe-init-build-env %s\"" % (self.pokydirname, self.be.builddir))
        # delete the templateconf.cfg; it may come from an unsupported layer configuration
        os.remove(os.path.join(self.be.builddir, "conf/templateconf.cfg"))


    def writeConfFile(self, file_name, variable_list = None, raw = None):
        filepath = os.path.join(self.be.builddir, file_name)
        with open(filepath, "w") as conffile:
            if variable_list is not None:
                for i in variable_list:
                    conffile.write("%s=\"%s\"\n" % (i.name, i.value))
            if raw is not None:
                conffile.write(raw)


    def startBBServer(self):
        assert self.pokydirname and os.path.exists(self.pokydirname)
        assert self.islayerset

        # find our own toasterui listener/bitbake
        from toaster.bldcontrol.management.commands.loadconf import _reduce_canon_path

        own_bitbake = _reduce_canon_path(os.path.join(os.path.dirname(os.path.abspath(__file__)), "../../../bin/bitbake"))

        assert os.path.exists(own_bitbake) and os.path.isfile(own_bitbake)

        logger.debug("localhostbecontroller: running the listener at %s" % own_bitbake)

        toaster_ui_log_filepath = os.path.join(self.be.builddir, "toaster_ui.log")
        # get the file length; we need to detect the _last_ start of the toaster UI, not the first
        toaster_ui_log_filelength = 0
        if os.path.exists(toaster_ui_log_filepath):
            with open(toaster_ui_log_filepath, "r") as f:
                f.seek(0, 2)    # jump to the end
                toaster_ui_log_filelength = f.tell()

        cmd = "bash -c \"source %s/oe-init-build-env %s 2>&1 >toaster_server.log && bitbake --read conf/toaster-pre.conf --postread conf/toaster.conf --server-only -t xmlrpc -B 0.0.0.0:0 2>&1 >toaster_server.log && DATABASE_URL=%s BBSERVER=0.0.0.0:-1 daemon -d -i -D %s -o toaster_ui.log -- %s --observe-only -u toasterui &\"" % (self.pokydirname, self.be.builddir,
                self.dburl, self.be.builddir, own_bitbake)
        port = "-1"
        logger.debug("localhostbecontroller: starting builder \n%s\n" % cmd)
        cmdoutput = self._shellcmd(cmd)
        for i in cmdoutput.split("\n"):
            if i.startswith("Bitbake server address"):
                port = i.split(" ")[-1]
                logger.debug("localhostbecontroller: Found bitbake server port %s" % port)

        def _toaster_ui_started(filepath, filepos = 0):
            if not os.path.exists(filepath):
                return False
            with open(filepath, "r") as f:
                f.seek(filepos)
                for line in f:
                    if line.startswith("NOTE: ToasterUI waiting for events"):
                        return True
            return False

        retries = 0
        started = False
        while not started and retries < 10:
            started = _toaster_ui_started(toaster_ui_log_filepath, toaster_ui_log_filelength)
            import time
            logger.debug("localhostbecontroller: Waiting bitbake server to start")
            time.sleep(0.5)
            retries += 1

        if not started:
            toaster_server_log = open(os.path.join(self.be.builddir, "toaster_server.log"), "r").read()
            raise BuildSetupException("localhostbecontroller: Bitbake server did not start in 5 seconds, aborting (Error: '%s' '%s')" % (cmdoutput, toaster_server_log))

        logger.debug("localhostbecontroller: Started bitbake server")

        while port == "-1":
            # the port specification is "autodetect"; read the bitbake.lock file
            with open("%s/bitbake.lock" % self.be.builddir, "r") as f:
                for line in f.readlines():
                    if ":" in line:
                        port = line.split(":")[1].strip()
                        logger.debug("localhostbecontroller: Autodetected bitbake port %s", port)
                        break

        assert self.be.sourcedir and os.path.exists(self.be.builddir)
        self.be.bbaddress = "localhost"
        self.be.bbport = port
        self.be.bbstate = BuildEnvironment.SERVER_STARTED
        self.be.save()

    def stopBBServer(self):
        assert self.pokydirname and os.path.exists(self.pokydirname)
        assert self.islayerset
        self._shellcmd("bash -c \"source %s/oe-init-build-env %s && %s source toaster stop\"" %
            (self.pokydirname, self.be.builddir, (lambda: "" if self.be.bbtoken is None else "BBTOKEN=%s" % self.be.bbtoken)()))
        self.be.bbstate = BuildEnvironment.SERVER_STOPPED
        self.be.save()
        logger.debug("localhostbecontroller: Stopped bitbake server")

    def getGitCloneDirectory(self, url, branch):
        """ Utility that returns the last component of a git path as directory
        """
        import re
        components = re.split(r'[:\.\/]', url)
        base = components[-2] if components[-1] == "git" else components[-1]

        if branch != "HEAD":
            return "_%s_%s.toaster_cloned" % (base, branch)


        # word of attention; this is a localhost-specific issue; only on the localhost we expect to have "HEAD" releases
        # which _ALWAYS_ means the current poky checkout
        from os.path import dirname as DN
        local_checkout_path = DN(DN(DN(DN(DN(os.path.abspath(__file__))))))
        #logger.debug("localhostbecontroller: using HEAD checkout in %s" % local_checkout_path)
        return local_checkout_path


    def setLayers(self, bitbakes, layers):
        """ a word of attention: by convention, the first layer for any build will be poky! """

        assert self.be.sourcedir is not None
        assert len(bitbakes) == 1
        # set layers in the layersource

        # 1. get a list of repos with branches, and map dirpaths for each layer
        gitrepos = {}

        gitrepos[(bitbakes[0].giturl, bitbakes[0].commit)] = []
        gitrepos[(bitbakes[0].giturl, bitbakes[0].commit)].append( ("bitbake", bitbakes[0].dirpath) )

        for layer in layers:
            # we don't process local URLs
            if layer.giturl.startswith("file://"):
                continue
            if not (layer.giturl, layer.commit) in gitrepos:
                gitrepos[(layer.giturl, layer.commit)] = []
            gitrepos[(layer.giturl, layer.commit)].append( (layer.name, layer.dirpath) )


        logger.debug("localhostbecontroller, our git repos are %s" % pformat(gitrepos))


        # 2. find checked-out git repos in the sourcedir directory that may help faster cloning

        cached_layers = {}
        for ldir in os.listdir(self.be.sourcedir):
            fldir = os.path.join(self.be.sourcedir, ldir)
            if os.path.isdir(fldir):
                try:
                    for line in self._shellcmd("git remote -v", fldir).split("\n"):
                        try:
                            remote = line.split("\t")[1].split(" ")[0]
                            if remote not in cached_layers:
                                cached_layers[remote] = fldir
                        except IndexError:
                            pass
                except ShellCmdException:
                    # ignore any errors in collecting git remotes
                    pass

        layerlist = []


        # 3. checkout the repositories
        for giturl, commit in gitrepos.keys():
            localdirname = os.path.join(self.be.sourcedir, self.getGitCloneDirectory(giturl, commit))
            logger.debug("localhostbecontroller: giturl %s:%s checking out in current directory %s" % (giturl, commit, localdirname))

            # make sure our directory is a git repository
            if os.path.exists(localdirname):
                localremotes = self._shellcmd("git remote -v", localdirname)
                if not giturl in localremotes:
                    raise BuildSetupException("Existing git repository at %s, but with different remotes ('%s', expected '%s'). Toaster will not continue out of fear of damaging something." % (localdirname, ", ".join(localremotes.split("\n")), giturl))
            else:
                if giturl in cached_layers:
                    logger.debug("localhostbecontroller git-copying %s to %s" % (cached_layers[giturl], localdirname))
                    self._shellcmd("git clone \"%s\" \"%s\"" % (cached_layers[giturl], localdirname))
                    self._shellcmd("git remote remove origin", localdirname)
                    self._shellcmd("git remote add origin \"%s\"" % giturl, localdirname)
                else:
                    logger.debug("localhostbecontroller: cloning %s:%s in %s" % (giturl, commit, localdirname))
                    self._shellcmd("git clone \"%s\" --single-branch --branch \"%s\" \"%s\"" % (giturl, commit, localdirname))

            # branch magic name "HEAD" will inhibit checkout
            if commit != "HEAD":
                logger.debug("localhostbecontroller: checking out commit %s to %s " % (commit, localdirname))
                self._shellcmd("git fetch --all && git checkout \"%s\" && git rebase \"origin/%s\"" % (commit, commit) , localdirname)

            # take the localdirname as poky dir if we can find the oe-init-build-env
            if self.pokydirname is None and os.path.exists(os.path.join(localdirname, "oe-init-build-env")):
                logger.debug("localhostbecontroller: selected poky dir name %s" % localdirname)
                self.pokydirname = localdirname

                # make sure we have a working bitbake
                if not os.path.exists(os.path.join(self.pokydirname, 'bitbake')):
                    logger.debug("localhostbecontroller: checking bitbake into the poky dirname %s " % self.pokydirname)
                    self._shellcmd("git clone -b \"%s\" \"%s\" \"%s\" " % (bitbakes[0].commit, bitbakes[0].giturl, os.path.join(self.pokydirname, 'bitbake')))

            # verify our repositories
            for name, dirpath in gitrepos[(giturl, commit)]:
                localdirpath = os.path.join(localdirname, dirpath)
                logger.debug("localhostbecontroller: localdirpath expected '%s'" % localdirpath)
                if not os.path.exists(localdirpath):
                    raise BuildSetupException("Cannot find layer git path '%s' in checked out repository '%s:%s'. Aborting." % (localdirpath, giturl, commit))

                if name != "bitbake":
                    layerlist.append(localdirpath.rstrip("/"))

        logger.debug("localhostbecontroller: current layer list %s " % pformat(layerlist))

        # 4. configure the build environment, so we have a conf/bblayers.conf
        assert self.pokydirname is not None
        self._setupBE()

        # 5. update the bblayers.conf
        bblayerconf = os.path.join(self.be.builddir, "conf/bblayers.conf")
        if not os.path.exists(bblayerconf):
            raise BuildSetupException("BE is not consistent: bblayers.conf file missing at %s" % bblayerconf)

        BuildEnvironmentController._updateBBLayers(bblayerconf, layerlist)

        self.islayerset = True
        return True

    def readServerLogFile(self):
        return open(os.path.join(self.be.builddir, "toaster_server.log"), "r").read()

    def release(self):
        assert self.be.sourcedir and os.path.exists(self.be.builddir)
        import shutil
        shutil.rmtree(os.path.join(self.be.sourcedir, "build"))
        assert not os.path.exists(self.be.builddir)
