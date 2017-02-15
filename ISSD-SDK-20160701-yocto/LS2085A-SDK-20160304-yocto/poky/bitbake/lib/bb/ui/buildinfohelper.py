#
# BitBake ToasterUI Implementation
#
# Copyright (C) 2013        Intel Corporation
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

import datetime
import sys
import bb
import re
import ast

os.environ["DJANGO_SETTINGS_MODULE"] = "toaster.toastermain.settings"

import toaster.toastermain.settings as toaster_django_settings
from toaster.orm.models import Build, Task, Recipe, Layer_Version, Layer, Target, LogMessage, HelpText
from toaster.orm.models import Target_Image_File, BuildArtifact
from toaster.orm.models import Variable, VariableHistory
from toaster.orm.models import Package, Package_File, Target_Installed_Package, Target_File
from toaster.orm.models import Task_Dependency, Package_Dependency
from toaster.orm.models import Recipe_Dependency
from bb.msg import BBLogFormatter as format
from django.db import models
from pprint import pformat
import logging

from django.db import transaction, connection

logger = logging.getLogger("BitBake")


class NotExisting(Exception):
    pass

class ORMWrapper(object):
    """ This class creates the dictionaries needed to store information in the database
        following the format defined by the Django models. It is also used to save this
        information in the database.
    """

    def __init__(self):
        self.layer_version_objects = []
        self.task_objects = {}
        self.recipe_objects = {}
        pass

    @staticmethod
    def _build_key(**kwargs):
        key = "0"
        for k in sorted(kwargs.keys()):
            if isinstance(kwargs[k], models.Model):
                key += "-%d" % kwargs[k].id
            else:
                key += "-%s" % str(kwargs[k])
        return key


    def _cached_get_or_create(self, clazz, **kwargs):
        """ This is a memory-cached get_or_create. We assume that the objects will not be created in the
            database through any other means.
        """

        assert issubclass(clazz, models.Model), "_cached_get_or_create needs to get the class as first argument"

        key = ORMWrapper._build_key(**kwargs)
        dictname = "objects_%s" % clazz.__name__
        if not dictname in vars(self).keys():
            vars(self)[dictname] = {}

        created = False
        if not key in vars(self)[dictname].keys():
            vars(self)[dictname][key] = clazz.objects.create(**kwargs)
            created = True

        return (vars(self)[dictname][key], created)


    def _cached_get(self, clazz, **kwargs):
        """ This is a memory-cached get. We assume that the objects will not change  in the database between gets.
        """
        assert issubclass(clazz, models.Model), "_cached_get needs to get the class as first argument"

        key = ORMWrapper._build_key(**kwargs)
        dictname = "objects_%s" % clazz.__name__

        if not dictname in vars(self).keys():
            vars(self)[dictname] = {}

        if not key in vars(self)[dictname].keys():
            vars(self)[dictname][key] = clazz.objects.get(**kwargs)

        return vars(self)[dictname][key]

    def create_build_object(self, build_info, brbe):
        assert 'machine' in build_info
        assert 'distro' in build_info
        assert 'distro_version' in build_info
        assert 'started_on' in build_info
        assert 'cooker_log_path' in build_info
        assert 'build_name' in build_info
        assert 'bitbake_version' in build_info

        build = Build.objects.create(
                                    machine=build_info['machine'],
                                    distro=build_info['distro'],
                                    distro_version=build_info['distro_version'],
                                    started_on=build_info['started_on'],
                                    completed_on=build_info['started_on'],
                                    cooker_log_path=build_info['cooker_log_path'],
                                    build_name=build_info['build_name'],
                                    bitbake_version=build_info['bitbake_version'])

        logger.debug(1, "buildinfohelper: build is created %s" % build)
        if brbe is not None:
            logger.debug(1, "buildinfohelper: brbe is %s" % brbe)
            from bldcontrol.models import BuildEnvironment, BuildRequest
            br, be = brbe.split(":")

            buildrequest = BuildRequest.objects.get(pk = br)
            buildrequest.build = build
            buildrequest.save()

            build.project_id = buildrequest.project_id
            build.save()
        return build

    def create_target_objects(self, target_info):
        assert 'build' in target_info
        assert 'targets' in target_info

        targets = []
        for tgt_name in target_info['targets']:
            tgt_object = Target.objects.create( build = target_info['build'],
                                    target = tgt_name,
                                    is_image = False,
                                    )
            targets.append(tgt_object)
        return targets

    def update_build_object(self, build, errors, warnings, taskfailures):
        assert isinstance(build,Build)
        assert isinstance(errors, int)
        assert isinstance(warnings, int)

        outcome = Build.SUCCEEDED
        if errors or taskfailures:
            outcome = Build.FAILED

        build.completed_on = datetime.datetime.now()
        build.timespent = int((build.completed_on - build.started_on).total_seconds())
        build.errors_no = errors
        build.warnings_no = warnings
        build.outcome = outcome
        build.save()

    def update_target_set_license_manifest(self, target, license_manifest_path):
        target.license_manifest_path = license_manifest_path
        target.save()

    def get_update_task_object(self, task_information, must_exist = False):
        assert 'build' in task_information
        assert 'recipe' in task_information
        assert 'task_name' in task_information

        # we use must_exist info for database look-up optimization
        task_object, created = self._cached_get_or_create(Task,
                        build=task_information['build'],
                        recipe=task_information['recipe'],
                        task_name=task_information['task_name']
                        )
        if created and must_exist:
                task_information['debug'] = "build id %d, recipe id %d" % (task_information['build'].pk, task_information['recipe'].pk)
                raise NotExisting("Task object created when expected to exist", task_information)

        object_changed = False
        for v in vars(task_object):
            if v in task_information.keys():
                if vars(task_object)[v] != task_information[v]:
                    vars(task_object)[v] = task_information[v]
                    object_changed = True

        # update setscene-related information if the task has a setscene
        if task_object.outcome == Task.OUTCOME_COVERED and 1 == task_object.get_related_setscene().count():
            task_object.outcome = Task.OUTCOME_CACHED
            object_changed = True

            outcome_task_setscene = Task.objects.get(task_executed=True, build = task_object.build,
                                    recipe = task_object.recipe, task_name=task_object.task_name+"_setscene").outcome
            if outcome_task_setscene == Task.OUTCOME_SUCCESS:
                task_object.sstate_result = Task.SSTATE_RESTORED
                object_changed = True
            elif outcome_task_setscene == Task.OUTCOME_FAILED:
                task_object.sstate_result = Task.SSTATE_FAILED
                object_changed = True

        # mark down duration if we have a start time and a current time
        if 'start_time' in task_information.keys() and 'end_time' in task_information.keys():
            duration = task_information['end_time'] - task_information['start_time']
            task_object.elapsed_time = duration
            object_changed = True
            del task_information['start_time']
            del task_information['end_time']

        if object_changed:
            task_object.save()
        return task_object


    def get_update_recipe_object(self, recipe_information, must_exist = False):
        assert 'layer_version' in recipe_information
        assert 'file_path' in recipe_information

        if recipe_information['file_path'].startswith(recipe_information['layer_version'].layer.local_path):
            recipe_information['file_path'] = recipe_information['file_path'][len(recipe_information['layer_version'].layer.local_path):].lstrip("/")

        recipe_object, created = self._cached_get_or_create(Recipe, layer_version=recipe_information['layer_version'],
                                     file_path=recipe_information['file_path'])
        if created and must_exist:
            raise NotExisting("Recipe object created when expected to exist", recipe_information)

        object_changed = False
        for v in vars(recipe_object):
            if v in recipe_information.keys():
                object_changed = True
                vars(recipe_object)[v] = recipe_information[v]

        if object_changed:
            recipe_object.save()

        return recipe_object

    def get_update_layer_version_object(self, build_obj, layer_obj, layer_version_information):
        assert isinstance(build_obj, Build)
        assert isinstance(layer_obj, Layer)
        assert 'branch' in layer_version_information
        assert 'commit' in layer_version_information
        assert 'priority' in layer_version_information

        layer_version_object, created = Layer_Version.objects.get_or_create(
                                    build = build_obj,
                                    layer = layer_obj,
                                    branch = layer_version_information['branch'],
                                    commit = layer_version_information['commit'],
                                    priority = layer_version_information['priority']
                                    )

        self.layer_version_objects.append(layer_version_object)

        return layer_version_object

    def get_update_layer_object(self, layer_information, brbe):
        assert 'name' in layer_information
        assert 'local_path' in layer_information
        assert 'layer_index_url' in layer_information

        if brbe is None:
            layer_object, created = Layer.objects.get_or_create(
                                name=layer_information['name'],
                                local_path=layer_information['local_path'],
                                layer_index_url=layer_information['layer_index_url'])
            return layer_object
        else:
            # we are under managed mode; we must match the layer used in the Project Layer
            from bldcontrol.models import BuildEnvironment, BuildRequest
            br_id, be_id = brbe.split(":")

            # find layer by checkout path;
            from bldcontrol import bbcontroller
            bc = bbcontroller.getBuildEnvironmentController(pk = be_id)

            # we might have a race condition here, as the project layers may change between the build trigger and the actual build execution
            # but we can only match on the layer name, so the worst thing can happen is a mis-identification of the layer, not a total failure

            # note that this is different
            buildrequest = BuildRequest.objects.get(pk = br_id)
            for brl in buildrequest.brlayer_set.all():
                localdirname = os.path.join(bc.getGitCloneDirectory(brl.giturl, brl.commit), brl.dirpath)
                # we get a relative path, unless running in HEAD mode where the path is absolute
                if not localdirname.startswith("/"):
                    localdirname = os.path.join(bc.be.sourcedir, localdirname)
                #logger.debug(1, "Localdirname %s lcal_path %s" % (localdirname, layer_information['local_path']))
                if localdirname.startswith(layer_information['local_path']):
                    # we matched the BRLayer, but we need the layer_version that generated this BR; reverse of the Project.schedule_build()
                    #logger.debug(1, "Matched %s to BRlayer %s" % (pformat(layer_information["local_path"]), localdirname))
                    for pl in buildrequest.project.projectlayer_set.filter(layercommit__layer__name = brl.name):
                        if pl.layercommit.layer.vcs_url == brl.giturl :
                            layer = pl.layercommit.layer
                            layer.local_path = layer_information['local_path']
                            layer.save()
                            return layer

            raise NotExisting("Unidentified layer %s" % pformat(layer_information))


    def save_target_file_information(self, build_obj, target_obj, filedata):
        assert isinstance(build_obj, Build)
        assert isinstance(target_obj, Target)
        dirs = filedata['dirs']
        files = filedata['files']
        syms = filedata['syms']

        # we insert directories, ordered by name depth
        for d in sorted(dirs, key=lambda x:len(x[-1].split("/"))):
            (user, group, size) = d[1:4]
            permission = d[0][1:]
            path = d[4].lstrip(".")
            if len(path) == 0:
                # we create the root directory as a special case
                path = "/"
                tf_obj = Target_File.objects.create(
                        target = target_obj,
                        path = path,
                        size = size,
                        inodetype = Target_File.ITYPE_DIRECTORY,
                        permission = permission,
                        owner = user,
                        group = group,
                        )
                tf_obj.directory = tf_obj
                tf_obj.save()
                continue
            parent_path = "/".join(path.split("/")[:len(path.split("/")) - 1])
            if len(parent_path) == 0:
                parent_path = "/"
            parent_obj = self._cached_get(Target_File, target = target_obj, path = parent_path, inodetype = Target_File.ITYPE_DIRECTORY)
            tf_obj = Target_File.objects.create(
                        target = target_obj,
                        path = path,
                        size = size,
                        inodetype = Target_File.ITYPE_DIRECTORY,
                        permission = permission,
                        owner = user,
                        group = group,
                        directory = parent_obj)


        # we insert files
        for d in files:
            (user, group, size) = d[1:4]
            permission = d[0][1:]
            path = d[4].lstrip(".")
            parent_path = "/".join(path.split("/")[:len(path.split("/")) - 1])
            inodetype = Target_File.ITYPE_REGULAR
            if d[0].startswith('b'):
                inodetype = Target_File.ITYPE_BLOCK
            if d[0].startswith('c'):
                inodetype = Target_File.ITYPE_CHARACTER
            if d[0].startswith('p'):
                inodetype = Target_File.ITYPE_FIFO

            tf_obj = Target_File.objects.create(
                        target = target_obj,
                        path = path,
                        size = size,
                        inodetype = inodetype,
                        permission = permission,
                        owner = user,
                        group = group)
            parent_obj = self._cached_get(Target_File, target = target_obj, path = parent_path, inodetype = Target_File.ITYPE_DIRECTORY)
            tf_obj.directory = parent_obj
            tf_obj.save()

        # we insert symlinks
        for d in syms:
            (user, group, size) = d[1:4]
            permission = d[0][1:]
            path = d[4].lstrip(".")
            filetarget_path = d[6]

            parent_path = "/".join(path.split("/")[:len(path.split("/")) - 1])
            if not filetarget_path.startswith("/"):
                # we have a relative path, get a normalized absolute one
                filetarget_path = parent_path + "/" + filetarget_path
                fcp = filetarget_path.split("/")
                fcpl = []
                for i in fcp:
                    if i == "..":
                        fcpl.pop()
                    else:
                        fcpl.append(i)
                filetarget_path = "/".join(fcpl)

            try:
                filetarget_obj = Target_File.objects.get(target = target_obj, path = filetarget_path)
            except Exception as e:
                # we might have an invalid link; no way to detect this. just set it to None
                filetarget_obj = None

            parent_obj = Target_File.objects.get(target = target_obj, path = parent_path, inodetype = Target_File.ITYPE_DIRECTORY)

            tf_obj = Target_File.objects.create(
                        target = target_obj,
                        path = path,
                        size = size,
                        inodetype = Target_File.ITYPE_SYMLINK,
                        permission = permission,
                        owner = user,
                        group = group,
                        directory = parent_obj,
                        sym_target = filetarget_obj)


    def save_target_package_information(self, build_obj, target_obj, packagedict, pkgpnmap, recipes):
        assert isinstance(build_obj, Build)
        assert isinstance(target_obj, Target)

        errormsg = ""
        for p in packagedict:
            searchname = p
            if p not in pkgpnmap:
                logger.warning("Image packages list contains %p, but is"
                               " missing from all packages list where the"
                               " metadata comes from.  Skipping...", p)
                continue
            if 'OPKGN' in pkgpnmap[p].keys():
                searchname = pkgpnmap[p]['OPKGN']

            packagedict[p]['object'], created = Package.objects.get_or_create( build = build_obj, name = searchname )
            if created or packagedict[p]['object'].size == -1:    # save the data anyway we can, not just if it was not created here; bug [YOCTO #6887]
                # fill in everything we can from the runtime-reverse package data
                try:
                    packagedict[p]['object'].recipe = recipes[pkgpnmap[p]['PN']]
                    packagedict[p]['object'].version = pkgpnmap[p]['PV']
                    packagedict[p]['object'].installed_name = p
                    packagedict[p]['object'].revision = pkgpnmap[p]['PR']
                    packagedict[p]['object'].license = pkgpnmap[p]['LICENSE']
                    packagedict[p]['object'].section = pkgpnmap[p]['SECTION']
                    packagedict[p]['object'].summary = pkgpnmap[p]['SUMMARY']
                    packagedict[p]['object'].description = pkgpnmap[p]['DESCRIPTION']
                    packagedict[p]['object'].size = int(pkgpnmap[p]['PKGSIZE'])

                # no files recorded for this package, so save files info
                    packagefile_objects = []
                    for targetpath in pkgpnmap[p]['FILES_INFO']:
                        targetfilesize = pkgpnmap[p]['FILES_INFO'][targetpath]
                        packagefile_objects.append(Package_File( package = packagedict[p]['object'],
                            path = targetpath,
                            size = targetfilesize))
                    if len(packagefile_objects):
                        Package_File.objects.bulk_create(packagefile_objects)
                except KeyError as e:
                    errormsg += "  stpi: Key error, package %s key %s \n" % ( p, e )

            # save disk installed size
            packagedict[p]['object'].installed_size = packagedict[p]['size']
            packagedict[p]['object'].save()

            Target_Installed_Package.objects.create(target = target_obj, package = packagedict[p]['object'])

        packagedeps_objs = []
        for p in packagedict:
            for (px,deptype) in packagedict[p]['depends']:
                if deptype == 'depends':
                    tdeptype = Package_Dependency.TYPE_TRDEPENDS
                elif deptype == 'recommends':
                    tdeptype = Package_Dependency.TYPE_TRECOMMENDS

                try:
                    packagedeps_objs.append(Package_Dependency(
                        package = packagedict[p]['object'],
                        depends_on = packagedict[px]['object'],
                        dep_type = tdeptype,
                        target = target_obj))
                except KeyError as e:
                    logger.warn("Could not add dependency to the package %s "
                                "because %s is an unknown package", p, px)

        if len(packagedeps_objs) > 0:
            Package_Dependency.objects.bulk_create(packagedeps_objs)
        else:
            logger.info("No package dependencies created")

        if (len(errormsg) > 0):
            logger.warn("buildinfohelper: target_package_info could not identify recipes: \n%s" % errormsg)

    def save_target_image_file_information(self, target_obj, file_name, file_size):
        target_image_file = Target_Image_File.objects.create( target = target_obj,
                            file_name = file_name,
                            file_size = file_size)

    def save_artifact_information(self, build_obj, file_name, file_size):
        # we skip the image files from other builds
        if Target_Image_File.objects.filter(file_name = file_name).count() > 0:
            return

        # do not update artifacts found in other builds
        if BuildArtifact.objects.filter(file_name = file_name).count() > 0:
            return

        BuildArtifact.objects.create(build = build_obj, file_name = file_name, file_size = file_size)

    def create_logmessage(self, log_information):
        assert 'build' in log_information
        assert 'level' in log_information
        assert 'message' in log_information

        log_object = LogMessage.objects.create(
                        build = log_information['build'],
                        level = log_information['level'],
                        message = log_information['message'])

        for v in vars(log_object):
            if v in log_information.keys():
                vars(log_object)[v] = log_information[v]

        return log_object.save()


    def save_build_package_information(self, build_obj, package_info, recipes):
        assert isinstance(build_obj, Build)

        # create and save the object
        pname = package_info['PKG']
        if 'OPKGN' in package_info.keys():
            pname = package_info['OPKGN']

        bp_object, created = Package.objects.get_or_create( build = build_obj,
                                       name = pname )

        bp_object.installed_name = package_info['PKG']
        bp_object.recipe = recipes[package_info['PN']]
        bp_object.version = package_info['PKGV']
        bp_object.revision = package_info['PKGR']
        bp_object.summary = package_info['SUMMARY']
        bp_object.description = package_info['DESCRIPTION']
        bp_object.size = int(package_info['PKGSIZE'])
        bp_object.section = package_info['SECTION']
        bp_object.license = package_info['LICENSE']
        bp_object.save()

        # save any attached file information
        packagefile_objects = []
        for path in package_info['FILES_INFO']:
                packagefile_objects.append(Package_File( package = bp_object,
                                        path = path,
                                        size = package_info['FILES_INFO'][path] ))
        if len(packagefile_objects):
            Package_File.objects.bulk_create(packagefile_objects)

        def _po_byname(p):
            pkg, created = Package.objects.get_or_create(build = build_obj, name = p)
            if created:
                pkg.size = -1
                pkg.save()
            return pkg

        packagedeps_objs = []
        # save soft dependency information
        if 'RDEPENDS' in package_info and package_info['RDEPENDS']:
            for p in bb.utils.explode_deps(package_info['RDEPENDS']):
                packagedeps_objs.append(Package_Dependency(  package = bp_object,
                    depends_on = _po_byname(p), dep_type = Package_Dependency.TYPE_RDEPENDS))
        if 'RPROVIDES' in package_info and package_info['RPROVIDES']:
            for p in bb.utils.explode_deps(package_info['RPROVIDES']):
                packagedeps_objs.append(Package_Dependency(  package = bp_object,
                    depends_on = _po_byname(p), dep_type = Package_Dependency.TYPE_RPROVIDES))
        if 'RRECOMMENDS' in package_info and package_info['RRECOMMENDS']:
            for p in bb.utils.explode_deps(package_info['RRECOMMENDS']):
                packagedeps_objs.append(Package_Dependency(  package = bp_object,
                    depends_on = _po_byname(p), dep_type = Package_Dependency.TYPE_RRECOMMENDS))
        if 'RSUGGESTS' in package_info and package_info['RSUGGESTS']:
            for p in bb.utils.explode_deps(package_info['RSUGGESTS']):
                packagedeps_objs.append(Package_Dependency(  package = bp_object,
                    depends_on = _po_byname(p), dep_type = Package_Dependency.TYPE_RSUGGESTS))
        if 'RREPLACES' in package_info and package_info['RREPLACES']:
            for p in bb.utils.explode_deps(package_info['RREPLACES']):
                packagedeps_objs.append(Package_Dependency(  package = bp_object,
                    depends_on = _po_byname(p), dep_type = Package_Dependency.TYPE_RREPLACES))
        if 'RCONFLICTS' in package_info and package_info['RCONFLICTS']:
            for p in bb.utils.explode_deps(package_info['RCONFLICTS']):
                packagedeps_objs.append(Package_Dependency(  package = bp_object,
                    depends_on = _po_byname(p), dep_type = Package_Dependency.TYPE_RCONFLICTS))

        if len(packagedeps_objs) > 0:
            Package_Dependency.objects.bulk_create(packagedeps_objs)

        return bp_object

    def save_build_variables(self, build_obj, vardump):
        assert isinstance(build_obj, Build)

        helptext_objects = []
        for k in vardump:
            desc = vardump[k]['doc']
            if desc is None:
                var_words = [word for word in k.split('_')]
                root_var = "_".join([word for word in var_words if word.isupper()])
                if root_var and root_var != k and root_var in vardump:
                    desc = vardump[root_var]['doc']
            if desc is None:
                desc = ''
            if len(desc):
                helptext_objects.append(HelpText(build=build_obj,
                    area=HelpText.VARIABLE,
                    key=k,
                    text=desc))
            if not bool(vardump[k]['func']):
                value = vardump[k]['v']
                if value is None:
                    value = ''
                variable_obj = Variable.objects.create( build = build_obj,
                    variable_name = k,
                    variable_value = value,
                    description = desc)

                varhist_objects = []
                for vh in vardump[k]['history']:
                    if not 'documentation.conf' in vh['file']:
                        varhist_objects.append(VariableHistory( variable = variable_obj,
                                file_name = vh['file'],
                                line_number = vh['line'],
                                operation = vh['op']))
                if len(varhist_objects):
                    VariableHistory.objects.bulk_create(varhist_objects)

        HelpText.objects.bulk_create(helptext_objects)

class MockEvent: pass           # sometimes we mock an event, declare it here

class BuildInfoHelper(object):
    """ This class gathers the build information from the server and sends it
        towards the ORM wrapper for storing in the database
        It is instantiated once per build
        Keeps in memory all data that needs matching before writing it to the database
    """


    def __init__(self, server, has_build_history = False):
        self._configure_django()
        self.internal_state = {}
        self.internal_state['taskdata'] = {}
        self.task_order = 0
        self.autocommit_step = 1
        self.server = server
        # we use manual transactions if the database doesn't autocommit on us
        if not connection.features.autocommits_when_autocommit_is_off:
            transaction.set_autocommit(False)
        self.orm_wrapper = ORMWrapper()
        self.has_build_history = has_build_history
        self.tmp_dir = self.server.runCommand(["getVariable", "TMPDIR"])[0]
        self.brbe    = self.server.runCommand(["getVariable", "TOASTER_BRBE"])[0]
        logger.debug(1, "buildinfohelper: Build info helper inited %s" % vars(self))


    def _configure_django(self):
        # Add toaster to sys path for importing modules
        sys.path.append(os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(__file__))), 'toaster'))

    ###################
    ## methods to convert event/external info into objects that the ORM layer uses


    def _get_build_information(self):
        build_info = {}
        # Generate an identifier for each new build

        build_info['machine'] = self.server.runCommand(["getVariable", "MACHINE"])[0]
        build_info['distro'] = self.server.runCommand(["getVariable", "DISTRO"])[0]
        build_info['distro_version'] = self.server.runCommand(["getVariable", "DISTRO_VERSION"])[0]
        build_info['started_on'] = datetime.datetime.now()
        build_info['completed_on'] = datetime.datetime.now()
        build_info['cooker_log_path'] = self.server.runCommand(["getVariable", "BB_CONSOLELOG"])[0]
        build_info['build_name'] = self.server.runCommand(["getVariable", "BUILDNAME"])[0]
        build_info['bitbake_version'] = self.server.runCommand(["getVariable", "BB_VERSION"])[0]

        return build_info

    def _get_task_information(self, event, recipe):
        assert 'taskname' in vars(event)

        task_information = {}
        task_information['build'] = self.internal_state['build']
        task_information['outcome'] = Task.OUTCOME_NA
        task_information['recipe'] = recipe
        task_information['task_name'] = event.taskname
        try:
            # some tasks don't come with a hash. and that's ok
            task_information['sstate_checksum'] = event.taskhash
        except AttributeError:
            pass
        return task_information

    def _get_layer_version_for_path(self, path):
        assert path.startswith("/")
        assert 'build' in self.internal_state

        if self.brbe is None:
            def _slkey_interactive(layer_version):
                assert isinstance(layer_version, Layer_Version)
                return len(layer_version.layer.local_path)

            # Heuristics: we always match recipe to the deepest layer path in the discovered layers
            for lvo in sorted(self.orm_wrapper.layer_version_objects, reverse=True, key=_slkey_interactive):
                # we can match to the recipe file path
                if path.startswith(lvo.layer.local_path):
                    return lvo

        else:
            br_id, be_id = self.brbe.split(":")
            from bldcontrol.bbcontroller import getBuildEnvironmentController
            from bldcontrol.models import BuildRequest
            bc = getBuildEnvironmentController(pk = be_id)

            def _slkey_managed(layer_version):
                return len(bc.getGitCloneDirectory(layer_version.giturl, layer_version.commit) + layer_version.dirpath)

            # Heuristics: we match the path to where the layers have been checked out
            for brl in sorted(BuildRequest.objects.get(pk = br_id).brlayer_set.all(), reverse = True, key = _slkey_managed):
                localdirname = os.path.join(bc.getGitCloneDirectory(brl.giturl, brl.commit), brl.dirpath)
                # we get a relative path, unless running in HEAD mode where the path is absolute
                if not localdirname.startswith("/"):
                    localdirname = os.path.join(bc.be.sourcedir, localdirname)
                if path.startswith(localdirname):
                    #logger.warn("-- managed: matched path %s with layer %s " % (path, localdirname))
                    # we matched the BRLayer, but we need the layer_version that generated this br
                    for lvo in self.orm_wrapper.layer_version_objects:
                        if brl.name == lvo.layer.name:
                            return lvo

        #if we get here, we didn't read layers correctly; dump whatever information we have on the error log
        logger.error("Could not match layer version for recipe path %s : %s" % (path, self.orm_wrapper.layer_version_objects))

        #mockup the new layer
        unknown_layer, created = Layer.objects.get_or_create(name="__FIXME__unidentified_layer", local_path="/", layer_index_url="")
        unknown_layer_version_obj, created = Layer_Version.objects.get_or_create(layer = unknown_layer, build = self.internal_state['build'])

        return unknown_layer_version_obj

    def _get_recipe_information_from_taskfile(self, taskfile):
        localfilepath = taskfile.split(":")[-1]
        layer_version_obj = self._get_layer_version_for_path(localfilepath)

        recipe_info = {}
        recipe_info['layer_version'] = layer_version_obj
        recipe_info['file_path'] = taskfile

        return recipe_info

    def _get_path_information(self, task_object):
        assert isinstance(task_object, Task)
        build_stats_format = "{tmpdir}/buildstats/{target}-{machine}/{buildname}/{package}/"
        build_stats_path = []

        for t in self.internal_state['targets']:
            target = t.target
            machine = self.internal_state['build'].machine
            buildname = self.internal_state['build'].build_name
            pe, pv = task_object.recipe.version.split(":",1)
            if len(pe) > 0:
                package = task_object.recipe.name + "-" + pe + "_" + pv
            else:
                package = task_object.recipe.name + "-" + pv

            build_stats_path.append(build_stats_format.format(tmpdir=self.tmp_dir, target=target,
                                                     machine=machine, buildname=buildname,
                                                     package=package))

        return build_stats_path

    def _remove_redundant(self, string):
        ret = []
        for i in string.split():
            if i not in ret:
                ret.append(i)
        return " ".join(sorted(ret))


    ################################
    ## external available methods to store information
    @staticmethod
    def _get_data_from_event(event):
        evdata = None
        if '_localdata' in vars(event):
            evdata = event._localdata
        elif 'data' in vars(event):
            evdata = event.data
        else:
            raise Exception("Event with neither _localdata or data properties")
        return evdata

    def store_layer_info(self, event):
        layerinfos = BuildInfoHelper._get_data_from_event(event)
        self.internal_state['lvs'] = {}
        for layer in layerinfos:
            try:
                self.internal_state['lvs'][self.orm_wrapper.get_update_layer_object(layerinfos[layer], self.brbe)] = layerinfos[layer]['version']
            except NotExisting as nee:
                logger.warn("buildinfohelper: cannot identify layer exception:%s " % nee)


    def store_started_build(self, event):
        assert '_pkgs' in vars(event)
        build_information = self._get_build_information()

        build_obj = self.orm_wrapper.create_build_object(build_information, self.brbe)

        self.internal_state['build'] = build_obj

        # save layer version information for this build
        if not 'lvs' in self.internal_state:
            logger.error("Layer version information not found; Check if the bitbake server was configured to inherit toaster.bbclass.")
        else:
            for layer_obj in self.internal_state['lvs']:
                self.orm_wrapper.get_update_layer_version_object(build_obj, layer_obj, self.internal_state['lvs'][layer_obj])

            del self.internal_state['lvs']

        # create target information
        target_information = {}
        target_information['targets'] = event._pkgs
        target_information['build'] = build_obj

        self.internal_state['targets'] = self.orm_wrapper.create_target_objects(target_information)

        # Save build configuration
        data = self.server.runCommand(["getAllKeysWithFlags", ["doc", "func"]])[0]
        self.orm_wrapper.save_build_variables(build_obj, data)

        return self.brbe


    def update_target_image_file(self, event):
        image_fstypes = self.server.runCommand(["getVariable", "IMAGE_FSTYPES"])[0]
        evdata = BuildInfoHelper._get_data_from_event(event)

        for t in self.internal_state['targets']:
            if t.is_image == True:
                output_files = list(evdata.viewkeys())
                for output in output_files:
                    if t.target in output and 'rootfs' in output and not output.endswith(".manifest"):
                        self.orm_wrapper.save_target_image_file_information(t, output, evdata[output])

    def update_artifact_image_file(self, event):
        evdata = BuildInfoHelper._get_data_from_event(event)
        for artifact_path in evdata.keys():
            self.orm_wrapper.save_artifact_information(self.internal_state['build'], artifact_path, evdata[artifact_path])

    def update_build_information(self, event, errors, warnings, taskfailures):
        if 'build' in self.internal_state:
            self.orm_wrapper.update_build_object(self.internal_state['build'], errors, warnings, taskfailures)


    def store_license_manifest_path(self, event):
        deploy_dir = BuildInfoHelper._get_data_from_event(event)['deploy_dir']
        image_name = BuildInfoHelper._get_data_from_event(event)['image_name']
        path = deploy_dir + "/licenses/" + image_name + "/license.manifest"
        for target in self.internal_state['targets']:
            if target.target in image_name:
                self.orm_wrapper.update_target_set_license_manifest(target, path)


    def store_started_task(self, event):
        assert isinstance(event, (bb.runqueue.sceneQueueTaskStarted, bb.runqueue.runQueueTaskStarted, bb.runqueue.runQueueTaskSkipped))
        assert 'taskfile' in vars(event)
        localfilepath = event.taskfile.split(":")[-1]
        assert localfilepath.startswith("/")

        identifier = event.taskfile + ":" + event.taskname

        recipe_information = self._get_recipe_information_from_taskfile(event.taskfile)
        recipe = self.orm_wrapper.get_update_recipe_object(recipe_information, True)

        task_information = self._get_task_information(event, recipe)
        task_information['outcome'] = Task.OUTCOME_NA

        if isinstance(event, bb.runqueue.runQueueTaskSkipped):
            assert 'reason' in vars(event)
            task_information['task_executed'] = False
            if event.reason == "covered":
                task_information['outcome'] = Task.OUTCOME_COVERED
            if event.reason == "existing":
                task_information['outcome'] = Task.OUTCOME_PREBUILT
        else:
            task_information['task_executed'] = True
            if 'noexec' in vars(event) and event.noexec == True:
                task_information['task_executed'] = False
                task_information['outcome'] = Task.OUTCOME_EMPTY
                task_information['script_type'] = Task.CODING_NA

        # do not assign order numbers to scene tasks
        if not isinstance(event, bb.runqueue.sceneQueueTaskStarted):
            self.task_order += 1
            task_information['order'] = self.task_order

        task_obj = self.orm_wrapper.get_update_task_object(task_information)

        self.internal_state['taskdata'][identifier] = {
                        'outcome': task_information['outcome'],
                    }


    def store_tasks_stats(self, event):
        for (taskfile, taskname, taskstats, recipename) in BuildInfoHelper._get_data_from_event(event):
            localfilepath = taskfile.split(":")[-1]
            assert localfilepath.startswith("/")

            recipe_information = self._get_recipe_information_from_taskfile(taskfile)
            try:
                if recipe_information['file_path'].startswith(recipe_information['layer_version'].layer.local_path):
                    recipe_information['file_path'] = recipe_information['file_path'][len(recipe_information['layer_version'].layer.local_path):].lstrip("/")

                recipe_object = Recipe.objects.get(layer_version = recipe_information['layer_version'],
                            file_path__endswith = recipe_information['file_path'],
                            name = recipename)
            except Recipe.DoesNotExist:
                logger.error("Could not find recipe for recipe_information %s name %s" % (pformat(recipe_information), recipename))
                raise

            task_information = {}
            task_information['build'] = self.internal_state['build']
            task_information['recipe'] = recipe_object
            task_information['task_name'] = taskname
            task_information['cpu_usage'] = taskstats['cpu_usage']
            task_information['disk_io'] = taskstats['disk_io']
            if 'elapsed_time' in taskstats:
                task_information['elapsed_time'] = taskstats['elapsed_time']
            task_obj = self.orm_wrapper.get_update_task_object(task_information, True)  # must exist

    def update_and_store_task(self, event):
        assert 'taskfile' in vars(event)
        localfilepath = event.taskfile.split(":")[-1]
        assert localfilepath.startswith("/")

        identifier = event.taskfile + ":" + event.taskname
        if not identifier in self.internal_state['taskdata']:
            if isinstance(event, bb.build.TaskBase):
                # we do a bit of guessing
                candidates = [x for x in self.internal_state['taskdata'].keys() if x.endswith(identifier)]
                if len(candidates) == 1:
                    identifier = candidates[0]

        assert identifier in self.internal_state['taskdata']
        identifierlist = identifier.split(":")
        realtaskfile = ":".join(identifierlist[0:len(identifierlist)-1])
        recipe_information = self._get_recipe_information_from_taskfile(realtaskfile)
        recipe = self.orm_wrapper.get_update_recipe_object(recipe_information, True)
        task_information = self._get_task_information(event,recipe)

        if 'time' in vars(event):
            if not 'start_time' in self.internal_state['taskdata'][identifier]:
                self.internal_state['taskdata'][identifier]['start_time'] = event.time
            else:
                task_information['end_time'] = event.time
                task_information['start_time'] = self.internal_state['taskdata'][identifier]['start_time']

        task_information['outcome'] = self.internal_state['taskdata'][identifier]['outcome']

        if 'logfile' in vars(event):
            task_information['logfile'] = event.logfile

        if '_message' in vars(event):
            task_information['message'] = event._message

        if 'taskflags' in vars(event):
            # with TaskStarted, we get even more information
            if 'python' in event.taskflags.keys() and event.taskflags['python'] == '1':
                task_information['script_type'] = Task.CODING_PYTHON
            else:
                task_information['script_type'] = Task.CODING_SHELL

        if task_information['outcome'] == Task.OUTCOME_NA:
            if isinstance(event, (bb.runqueue.runQueueTaskCompleted, bb.runqueue.sceneQueueTaskCompleted)):
                task_information['outcome'] = Task.OUTCOME_SUCCESS
                del self.internal_state['taskdata'][identifier]

            if isinstance(event, (bb.runqueue.runQueueTaskFailed, bb.runqueue.sceneQueueTaskFailed)):
                task_information['outcome'] = Task.OUTCOME_FAILED
                del self.internal_state['taskdata'][identifier]

        if not connection.features.autocommits_when_autocommit_is_off:
            # we force a sync point here, to get the progress bar to show
            if self.autocommit_step % 3 == 0:
                transaction.set_autocommit(True)
                transaction.set_autocommit(False)
            self.autocommit_step += 1

        self.orm_wrapper.get_update_task_object(task_information, True) # must exist


    def store_missed_state_tasks(self, event):
        for (fn, taskname, taskhash, sstatefile) in BuildInfoHelper._get_data_from_event(event)['missed']:

            identifier = fn + taskname + "_setscene"
            recipe_information = self._get_recipe_information_from_taskfile(fn)
            recipe = self.orm_wrapper.get_update_recipe_object(recipe_information)
            mevent = MockEvent()
            mevent.taskname = taskname
            mevent.taskhash = taskhash
            task_information = self._get_task_information(mevent,recipe)

            task_information['start_time'] = datetime.datetime.now()
            task_information['outcome'] = Task.OUTCOME_NA
            task_information['sstate_checksum'] = taskhash
            task_information['sstate_result'] = Task.SSTATE_MISS
            task_information['path_to_sstate_obj'] = sstatefile

            self.orm_wrapper.get_update_task_object(task_information)

        for (fn, taskname, taskhash, sstatefile) in BuildInfoHelper._get_data_from_event(event)['found']:

            identifier = fn + taskname + "_setscene"
            recipe_information = self._get_recipe_information_from_taskfile(fn)
            recipe = self.orm_wrapper.get_update_recipe_object(recipe_information)
            mevent = MockEvent()
            mevent.taskname = taskname
            mevent.taskhash = taskhash
            task_information = self._get_task_information(mevent,recipe)

            task_information['path_to_sstate_obj'] = sstatefile

            self.orm_wrapper.get_update_task_object(task_information)


    def store_target_package_data(self, event):
        # for all image targets
        for target in self.internal_state['targets']:
            if target.is_image:
                pkgdata = BuildInfoHelper._get_data_from_event(event)['pkgdata']
                imgdata = BuildInfoHelper._get_data_from_event(event)['imgdata'][target.target]
                filedata = BuildInfoHelper._get_data_from_event(event)['filedata'][target.target]

                try:
                    self.orm_wrapper.save_target_package_information(self.internal_state['build'], target, imgdata, pkgdata, self.internal_state['recipes'])
                except KeyError as e:
                    logger.warn("KeyError in save_target_package_information"
                                "%s ", e)

                try:
                    self.orm_wrapper.save_target_file_information(self.internal_state['build'], target, filedata)
                except KeyError as e:
                    logger.warn("KeyError in save_target_file_information"
                                "%s ", e)

    def store_dependency_information(self, event):
        assert '_depgraph' in vars(event)
        assert 'layer-priorities' in event._depgraph
        assert 'pn' in event._depgraph
        assert 'tdepends' in event._depgraph

        errormsg = ""

        # save layer version priorities
        if 'layer-priorities' in event._depgraph.keys():
            for lv in event._depgraph['layer-priorities']:
                (name, path, regexp, priority) = lv
                layer_version_obj = self._get_layer_version_for_path(path[1:]) # paths start with a ^
                assert layer_version_obj is not None
                layer_version_obj.priority = priority
                layer_version_obj.save()

        # save recipe information
        self.internal_state['recipes'] = {}
        for pn in event._depgraph['pn']:

            file_name = event._depgraph['pn'][pn]['filename']
            layer_version_obj = self._get_layer_version_for_path(file_name.split(":")[-1])

            assert layer_version_obj is not None

            recipe_info = {}
            recipe_info['name'] = pn
            recipe_info['layer_version'] = layer_version_obj

            if 'version' in event._depgraph['pn'][pn]:
                recipe_info['version'] = event._depgraph['pn'][pn]['version'].lstrip(":")

            if 'summary' in event._depgraph['pn'][pn]:
                recipe_info['summary'] = event._depgraph['pn'][pn]['summary']

            if 'license' in event._depgraph['pn'][pn]:
                recipe_info['license'] = event._depgraph['pn'][pn]['license']

            if 'description' in event._depgraph['pn'][pn]:
                recipe_info['description'] = event._depgraph['pn'][pn]['description']

            if 'section' in event._depgraph['pn'][pn]:
                recipe_info['section'] = event._depgraph['pn'][pn]['section']

            if 'homepage' in event._depgraph['pn'][pn]:
                recipe_info['homepage'] = event._depgraph['pn'][pn]['homepage']

            if 'bugtracker' in event._depgraph['pn'][pn]:
                recipe_info['bugtracker'] = event._depgraph['pn'][pn]['bugtracker']

            recipe_info['file_path'] = file_name
            recipe = self.orm_wrapper.get_update_recipe_object(recipe_info)
            recipe.is_image = False
            if 'inherits' in event._depgraph['pn'][pn].keys():
                for cls in event._depgraph['pn'][pn]['inherits']:
                    if cls.endswith('/image.bbclass'):
                        recipe.is_image = True
                        break
            if recipe.is_image:
                for t in self.internal_state['targets']:
                    if pn == t.target:
                        t.is_image = True
                        t.save()
            self.internal_state['recipes'][pn] = recipe

        # we'll not get recipes for key w/ values listed in ASSUME_PROVIDED

        assume_provided = self.server.runCommand(["getVariable", "ASSUME_PROVIDED"])[0].split()

        # save recipe dependency
        # buildtime
        recipedeps_objects = []
        for recipe in event._depgraph['depends']:
            try:
                target = self.internal_state['recipes'][recipe]
                for dep in event._depgraph['depends'][recipe]:
                    dependency = self.internal_state['recipes'][dep]
                    recipedeps_objects.append(Recipe_Dependency( recipe = target,
                            depends_on = dependency, dep_type = Recipe_Dependency.TYPE_DEPENDS))
            except KeyError as e:
                if e not in assume_provided and not str(e).startswith("virtual/"):
                    errormsg += "  stpd: KeyError saving recipe dependency for %s, %s \n" % (recipe, e)
        Recipe_Dependency.objects.bulk_create(recipedeps_objects)

        # save all task information
        def _save_a_task(taskdesc):
            spec = re.split(r'\.', taskdesc)
            pn = ".".join(spec[0:-1])
            taskname = spec[-1]
            e = event
            e.taskname = pn
            recipe = self.internal_state['recipes'][pn]
            task_info = self._get_task_information(e, recipe)
            task_info['task_name'] = taskname
            task_obj = self.orm_wrapper.get_update_task_object(task_info)
            return task_obj

        # create tasks
        tasks = {}
        for taskdesc in event._depgraph['tdepends']:
            tasks[taskdesc] = _save_a_task(taskdesc)

        # create dependencies between tasks
        taskdeps_objects = []
        for taskdesc in event._depgraph['tdepends']:
            target = tasks[taskdesc]
            for taskdep in event._depgraph['tdepends'][taskdesc]:
                if taskdep not in tasks:
                    # Fetch tasks info is not collected previously
                    dep = _save_a_task(taskdep)
                else:
                    dep = tasks[taskdep]
                taskdeps_objects.append(Task_Dependency( task = target, depends_on = dep ))
        Task_Dependency.objects.bulk_create(taskdeps_objects)

        if (len(errormsg) > 0):
            logger.warn("buildinfohelper: dependency info not identify recipes: \n%s" % errormsg)


    def store_build_package_information(self, event):
        package_info = BuildInfoHelper._get_data_from_event(event)
        self.orm_wrapper.save_build_package_information(self.internal_state['build'],
                            package_info,
                            self.internal_state['recipes'],
                            )

    def _store_build_done(self, errorcode):
        br_id, be_id = self.brbe.split(":")
        from bldcontrol.models import BuildEnvironment, BuildRequest
        be = BuildEnvironment.objects.get(pk = be_id)
        be.lock = BuildEnvironment.LOCK_LOCK
        be.save()
        br = BuildRequest.objects.get(pk = br_id)
        if errorcode == 0:
            # request archival of the project artifacts
            br.state = BuildRequest.REQ_ARCHIVE
        else:
            br.state = BuildRequest.REQ_FAILED
        br.save()


    def store_log_error(self, text):
        mockevent = MockEvent()
        mockevent.levelno = format.ERROR
        mockevent.msg = text
        mockevent.pathname = '-- None'
        mockevent.lineno = -1
        self.store_log_event(mockevent)

    def store_log_exception(self, text, backtrace = ""):
        mockevent = MockEvent()
        mockevent.levelno = -1
        mockevent.msg = text
        mockevent.pathname = backtrace
        mockevent.lineno = -1
        self.store_log_event(mockevent)


    def store_log_event(self, event):
        if event.levelno < format.WARNING:
            return

        if 'args' in vars(event):
            event.msg = event.msg % event.args

        if not 'build' in self.internal_state:
            if self.brbe is None:
                if not 'backlog' in self.internal_state:
                    self.internal_state['backlog'] = []
                self.internal_state['backlog'].append(event)
            else:   # we're under Toaster control, post the errors to the build request
                from bldcontrol.models import BuildRequest, BRError
                br, be = self.brbe.split(":")
                buildrequest = BuildRequest.objects.get(pk = br)
                brerror = BRError.objects.create(req = buildrequest, errtype="build", errmsg = event.msg)

            return

        if 'build' in self.internal_state and 'backlog' in self.internal_state:
            # if we have a backlog of events, do our best to save them here
            if len(self.internal_state['backlog']):
                tempevent = self.internal_state['backlog'].pop()
                logger.debug(1, "buildinfohelper: Saving stored event %s " % tempevent)
                self.store_log_event(tempevent)
            else:
                logger.error("buildinfohelper: Events not saved: %s" % self.internal_state['backlog'])
                del self.internal_state['backlog']

        log_information = {}
        log_information['build'] = self.internal_state['build']
        if event.levelno == format.ERROR:
            log_information['level'] = LogMessage.ERROR
        elif event.levelno == format.WARNING:
            log_information['level'] = LogMessage.WARNING
        elif event.levelno == -1:   # toaster self-logging
            log_information['level'] = -1
        else:
            log_information['level'] = LogMessage.INFO

        log_information['message'] = event.msg
        log_information['pathname'] = event.pathname
        log_information['lineno'] = event.lineno
        self.orm_wrapper.create_logmessage(log_information)

    def close(self, errorcode):
        if self.brbe is not None:
            self._store_build_done(errorcode)

        if 'backlog' in self.internal_state:
            if 'build' in self.internal_state:
                # we save missed events in the database for the current build
                tempevent = self.internal_state['backlog'].pop()
                self.store_log_event(tempevent)
            else:
                # we have no build, and we still have events; something amazingly wrong happend
                for event in self.internal_state['backlog']:
                   logger.error("UNSAVED log: %s", event.msg)

        if not connection.features.autocommits_when_autocommit_is_off:
            transaction.set_autocommit(True)
