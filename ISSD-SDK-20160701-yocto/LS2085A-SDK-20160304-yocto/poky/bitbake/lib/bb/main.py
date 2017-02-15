#!/usr/bin/env python
# ex:ts=4:sw=4:sts=4:et
# -*- tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-
#
# Copyright (C) 2003, 2004  Chris Larson
# Copyright (C) 2003, 2004  Phil Blundell
# Copyright (C) 2003 - 2005 Michael 'Mickey' Lauer
# Copyright (C) 2005        Holger Hans Peter Freyther
# Copyright (C) 2005        ROAD GmbH
# Copyright (C) 2006        Richard Purdie
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
import logging
import optparse
import warnings

import bb
from bb import event
import bb.msg
from bb import cooker
from bb import ui
from bb import server
from bb import cookerdata

__version__ = "1.26.0"
logger = logging.getLogger("BitBake")

class BBMainException(bb.BBHandledException):
    pass

def get_ui(config):
    if not config.ui:
        # modify 'ui' attribute because it is also read by cooker
        config.ui = os.environ.get('BITBAKE_UI', 'knotty')

    interface = config.ui

    try:
        # Dynamically load the UI based on the ui name. Although we
        # suggest a fixed set this allows you to have flexibility in which
        # ones are available.
        module = __import__("bb.ui", fromlist = [interface])
        return getattr(module, interface)
    except AttributeError:
        raise BBMainException("FATAL: Invalid user interface '%s' specified.\n"
                 "Valid interfaces: depexp, goggle, ncurses, hob, knotty [default]." % interface)


# Display bitbake/OE warnings via the BitBake.Warnings logger, ignoring others"""
warnlog = logging.getLogger("BitBake.Warnings")
_warnings_showwarning = warnings.showwarning
def _showwarning(message, category, filename, lineno, file=None, line=None):
    if file is not None:
        if _warnings_showwarning is not None:
            _warnings_showwarning(message, category, filename, lineno, file, line)
    else:
        s = warnings.formatwarning(message, category, filename, lineno)
        warnlog.warn(s)

warnings.showwarning = _showwarning
warnings.filterwarnings("ignore")
warnings.filterwarnings("default", module="(<string>$|(oe|bb)\.)")
warnings.filterwarnings("ignore", category=PendingDeprecationWarning)
warnings.filterwarnings("ignore", category=ImportWarning)
warnings.filterwarnings("ignore", category=DeprecationWarning, module="<string>$")
warnings.filterwarnings("ignore", message="With-statements now directly support multiple context managers")

class BitBakeConfigParameters(cookerdata.ConfigParameters):

    def parseCommandLine(self, argv=sys.argv):
        parser = optparse.OptionParser(
            version = "BitBake Build Tool Core version %s, %%prog version %s" % (bb.__version__, __version__),
            usage = """%prog [options] [recipename/target ...]

    Executes the specified task (default is 'build') for a given set of target recipes (.bb files).
    It is assumed there is a conf/bblayers.conf available in cwd or in BBPATH which
    will provide the layer, BBFILES and other configuration information.""")

        parser.add_option("-b", "--buildfile", help = "Execute tasks from a specific .bb recipe directly. WARNING: Does not handle any dependencies from other recipes.",
                   action = "store", dest = "buildfile", default = None)

        parser.add_option("-k", "--continue", help = "Continue as much as possible after an error. While the target that failed and anything depending on it cannot be built, as much as possible will be built before stopping.",
                   action = "store_false", dest = "abort", default = True)

        parser.add_option("-a", "--tryaltconfigs", help = "Continue with builds by trying to use alternative providers where possible.",
                   action = "store_true", dest = "tryaltconfigs", default = False)

        parser.add_option("-f", "--force", help = "Force the specified targets/task to run (invalidating any existing stamp file).",
                   action = "store_true", dest = "force", default = False)

        parser.add_option("-c", "--cmd", help = "Specify the task to execute. The exact options available depend on the metadata. Some examples might be 'compile' or 'populate_sysroot' or 'listtasks' may give a list of the tasks available.",
                   action = "store", dest = "cmd")

        parser.add_option("-C", "--clear-stamp", help = "Invalidate the stamp for the specified task such as 'compile' and then run the default task for the specified target(s).",
                    action = "store", dest = "invalidate_stamp")

        parser.add_option("-r", "--read", help = "Read the specified file before bitbake.conf.",
                   action = "append", dest = "prefile", default = [])

        parser.add_option("-R", "--postread", help = "Read the specified file after bitbake.conf.",
                          action = "append", dest = "postfile", default = [])

        parser.add_option("-v", "--verbose", help = "Output more log message data to the terminal.",
                   action = "store_true", dest = "verbose", default = False)

        parser.add_option("-D", "--debug", help = "Increase the debug level. You can specify this more than once.",
                   action = "count", dest="debug", default = 0)

        parser.add_option("-n", "--dry-run", help = "Don't execute, just go through the motions.",
                   action = "store_true", dest = "dry_run", default = False)

        parser.add_option("-S", "--dump-signatures", help = "Dump out the signature construction information, with no task execution. The SIGNATURE_HANDLER parameter is passed to the handler. Two common values are none and printdiff but the handler may define more/less. none means only dump the signature, printdiff means compare the dumped signature with the cached one.",
                   action = "append", dest = "dump_signatures", default = [], metavar="SIGNATURE_HANDLER")

        parser.add_option("-p", "--parse-only", help = "Quit after parsing the BB recipes.",
                   action = "store_true", dest = "parse_only", default = False)

        parser.add_option("-s", "--show-versions", help = "Show current and preferred versions of all recipes.",
                   action = "store_true", dest = "show_versions", default = False)

        parser.add_option("-e", "--environment", help = "Show the global or per-recipe environment complete with information about where variables were set/changed.",
                   action = "store_true", dest = "show_environment", default = False)

        parser.add_option("-g", "--graphviz", help = "Save dependency tree information for the specified targets in the dot syntax.",
                    action = "store_true", dest = "dot_graph", default = False)

        parser.add_option("-I", "--ignore-deps", help = """Assume these dependencies don't exist and are already provided (equivalent to ASSUME_PROVIDED). Useful to make dependency graphs more appealing""",
                    action = "append", dest = "extra_assume_provided", default = [])

        parser.add_option("-l", "--log-domains", help = """Show debug logging for the specified logging domains""",
                    action = "append", dest = "debug_domains", default = [])

        parser.add_option("-P", "--profile", help = "Profile the command and save reports.",
                   action = "store_true", dest = "profile", default = False)

        parser.add_option("-u", "--ui", help = "The user interface to use (e.g. knotty, hob, depexp).",
                   action = "store", dest = "ui")

        parser.add_option("-t", "--servertype", help = "Choose which server to use, process or xmlrpc.",
                   action = "store", dest = "servertype")

        parser.add_option("", "--token", help = "Specify the connection token to be used when connecting to a remote server.",
                   action = "store", dest = "xmlrpctoken")

        parser.add_option("", "--revisions-changed", help = "Set the exit code depending on whether upstream floating revisions have changed or not.",
                   action = "store_true", dest = "revisions_changed", default = False)

        parser.add_option("", "--server-only", help = "Run bitbake without a UI, only starting a server (cooker) process.",
                   action = "store_true", dest = "server_only", default = False)

        parser.add_option("-B", "--bind", help = "The name/address for the bitbake server to bind to.",
                   action = "store", dest = "bind", default = False)

        parser.add_option("", "--no-setscene", help = "Do not run any setscene tasks. sstate will be ignored and everything needed, built.",
                   action = "store_true", dest = "nosetscene", default = False)

        parser.add_option("", "--remote-server", help = "Connect to the specified server.",
                   action = "store", dest = "remote_server", default = False)

        parser.add_option("-m", "--kill-server", help = "Terminate the remote server.",
                    action = "store_true", dest = "kill_server", default = False)

        parser.add_option("", "--observe-only", help = "Connect to a server as an observing-only client.",
                   action = "store_true", dest = "observe_only", default = False)

        parser.add_option("", "--status-only", help = "Check the status of the remote bitbake server.",
                   action = "store_true", dest = "status_only", default = False)

        parser.add_option("-w", "--write-log", help = "Writes the event log of the build to a bitbake event json file. Use '' (empty string) to assign the name automatically.",
                   action = "store", dest = "writeeventlog")

        options, targets = parser.parse_args(argv)

        # some environmental variables set also configuration options
        if "BBSERVER" in os.environ:
            options.servertype = "xmlrpc"
            options.remote_server = os.environ["BBSERVER"]

        if "BBTOKEN" in os.environ:
            options.xmlrpctoken = os.environ["BBTOKEN"]

        if "BBEVENTLOG" is os.environ:
            options.writeeventlog = os.environ["BBEVENTLOG"]

        # fill in proper log name if not supplied
        if options.writeeventlog is not None and len(options.writeeventlog) == 0:
            import datetime
            options.writeeventlog = "bitbake_eventlog_%s.json" % datetime.datetime.now().strftime("%Y%m%d%H%M%S")

        # if BBSERVER says to autodetect, let's do that
        if options.remote_server:
            [host, port] = options.remote_server.split(":", 2)
            port = int(port)
            # use automatic port if port set to -1, means read it from
            # the bitbake.lock file; this is a bit tricky, but we always expect
            # to be in the base of the build directory if we need to have a
            # chance to start the server later, anyway
            if port == -1:
                lock_location = "./bitbake.lock"
                # we try to read the address at all times; if the server is not started,
                # we'll try to start it after the first connect fails, below
                try:
                    lf = open(lock_location, 'r')
                    remotedef = lf.readline()
                    [host, port] = remotedef.split(":")
                    port = int(port)
                    lf.close()
                    options.remote_server = remotedef
                except Exception as e:
                    raise BBMainException("Failed to read bitbake.lock (%s), invalid port" % str(e))

        return options, targets[1:]


def start_server(servermodule, configParams, configuration, features):
    server = servermodule.BitBakeServer()
    if configParams.bind:
        (host, port) = configParams.bind.split(':')
        server.initServer((host, int(port)))
        configuration.interface = [ server.serverImpl.host, server.serverImpl.port ]
    else:
        server.initServer()
        configuration.interface = []

    try:
        configuration.setServerRegIdleCallback(server.getServerIdleCB())

        cooker = bb.cooker.BBCooker(configuration, features)

        server.addcooker(cooker)
        server.saveConnectionDetails()
    except Exception as e:
        exc_info = sys.exc_info()
        while hasattr(server, "event_queue"):
            try:
                import queue
            except ImportError:
                import Queue as queue
            try:
                event = server.event_queue.get(block=False)
            except (queue.Empty, IOError):
                break
            if isinstance(event, logging.LogRecord):
                logger.handle(event)
        raise exc_info[1], None, exc_info[2]
    server.detach()
    cooker.lock.close()
    return server


def bitbake_main(configParams, configuration):

    # Python multiprocessing requires /dev/shm on Linux
    if sys.platform.startswith('linux') and not os.access('/dev/shm', os.W_OK | os.X_OK):
        raise BBMainException("FATAL: /dev/shm does not exist or is not writable")

    # Unbuffer stdout to avoid log truncation in the event
    # of an unorderly exit as well as to provide timely
    # updates to log files for use with tail
    try:
        if sys.stdout.name == '<stdout>':
            sys.stdout = os.fdopen(sys.stdout.fileno(), 'w', 0)
    except:
        pass


    configuration.setConfigParameters(configParams)

    ui_module = get_ui(configParams)

    # Server type can be xmlrpc or process currently, if nothing is specified,
    # the default server is process
    if configParams.servertype:
        server_type = configParams.servertype
    else:
        server_type = 'process'

    try:
        module = __import__("bb.server", fromlist = [server_type])
        servermodule = getattr(module, server_type)
    except AttributeError:
        raise BBMainException("FATAL: Invalid server type '%s' specified.\n"
                              "Valid interfaces: xmlrpc, process [default]." % server_type)

    if configParams.server_only:
        if configParams.servertype != "xmlrpc":
            raise BBMainException("FATAL: If '--server-only' is defined, we must set the "
                                  "servertype as 'xmlrpc'.\n")
        if not configParams.bind:
            raise BBMainException("FATAL: The '--server-only' option requires a name/address "
                                  "to bind to with the -B option.\n")
        if configParams.remote_server:
            raise BBMainException("FATAL: The '--server-only' option conflicts with %s.\n" %
                                  ("the BBSERVER environment variable" if "BBSERVER" in os.environ \
                                   else "the '--remote-server' option" ))

    if configParams.bind and configParams.servertype != "xmlrpc":
        raise BBMainException("FATAL: If '-B' or '--bind' is defined, we must "
                              "set the servertype as 'xmlrpc'.\n")

    if configParams.remote_server and configParams.servertype != "xmlrpc":
        raise BBMainException("FATAL: If '--remote-server' is defined, we must "
                              "set the servertype as 'xmlrpc'.\n")

    if configParams.observe_only and (not configParams.remote_server or configParams.bind):
        raise BBMainException("FATAL: '--observe-only' can only be used by UI clients "
                              "connecting to a server.\n")

    if configParams.kill_server and not configParams.remote_server:
        raise BBMainException("FATAL: '--kill-server' can only be used to terminate a remote server")

    if "BBDEBUG" in os.environ:
        level = int(os.environ["BBDEBUG"])
        if level > configuration.debug:
            configuration.debug = level

    bb.msg.init_msgconfig(configParams.verbose, configuration.debug,
                         configuration.debug_domains)

    # Ensure logging messages get sent to the UI as events
    handler = bb.event.LogHandler()
    if not configParams.status_only:
        # In status only mode there are no logs and no UI
        logger.addHandler(handler)

    # Clear away any spurious environment variables while we stoke up the cooker
    cleanedvars = bb.utils.clean_environment()

    featureset = []
    if not configParams.server_only:
        # Collect the feature set for the UI
        featureset = getattr(ui_module, "featureSet", [])

    if not configParams.remote_server:
        # we start a server with a given configuration
        server = start_server(servermodule, configParams, configuration, featureset)
        bb.event.ui_queue = []
    else:
        # we start a stub server that is actually a XMLRPClient that connects to a real server
        server = servermodule.BitBakeXMLRPCClient(configParams.observe_only, configParams.xmlrpctoken)
        server.saveConnectionDetails(configParams.remote_server)


    if not configParams.server_only:
        try:
            server_connection = server.establishConnection(featureset)
        except Exception as e:
            if configParams.kill_server:
                return 0
            bb.fatal("Could not connect to server %s: %s" % (configParams.remote_server, str(e)))

        # Restore the environment in case the UI needs it
        for k in cleanedvars:
            os.environ[k] = cleanedvars[k]

        logger.removeHandler(handler)


        if configParams.status_only:
            server_connection.terminate()
            return 0

        if configParams.kill_server:
            server_connection.connection.terminateServer()
            bb.event.ui_queue = []
            return 0

        try:
            return ui_module.main(server_connection.connection, server_connection.events, configParams)
        finally:
            bb.event.ui_queue = []
            server_connection.terminate()
    else:
        print("Bitbake server address: %s, server port: %s" % (server.serverImpl.host, server.serverImpl.port))
        return 0

    return 1
