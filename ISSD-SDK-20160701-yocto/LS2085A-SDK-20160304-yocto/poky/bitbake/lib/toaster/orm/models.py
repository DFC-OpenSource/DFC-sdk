#
# ex:ts=4:sw=4:sts=4:et
# -*- tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-
#
# BitBake Toaster Implementation
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

from django.db import models
from django.db.models import F, Q
from django.utils import timezone


from django.core import validators
from django.conf import settings

class GitURLValidator(validators.URLValidator):
    import re
    regex = re.compile(
        r'^(?:ssh|git|http|ftp)s?://'  # http:// or https://
        r'(?:(?:[A-Z0-9](?:[A-Z0-9-]{0,61}[A-Z0-9])?\.)+(?:[A-Z]{2,6}\.?|[A-Z0-9-]{2,}\.?)|'  # domain...
        r'localhost|'  # localhost...
        r'\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}|'  # ...or ipv4
        r'\[?[A-F0-9]*:[A-F0-9:]+\]?)'  # ...or ipv6
        r'(?::\d+)?'  # optional port
        r'(?:/?|[/?]\S+)$', re.IGNORECASE)

def GitURLField(**kwargs):
    r = models.URLField(**kwargs)
    for i in xrange(len(r.validators)):
        if isinstance(r.validators[i], validators.URLValidator):
            r.validators[i] = GitURLValidator()
    return r


class ToasterSetting(models.Model):
    name = models.CharField(max_length=63)
    helptext = models.TextField()
    value = models.CharField(max_length=255)

    def __unicode__(self):
        return "Setting %s = " % (self.name, self.value)

class ProjectManager(models.Manager):
    def create_project(self, name, release):
        prj = self.model(name = name, bitbake_version = release.bitbake_version, release = release)
        prj.save()

        for defaultconf in ToasterSetting.objects.filter(name__startswith="DEFCONF_"):
            name = defaultconf.name[8:]
            ProjectVariable.objects.create( project = prj,
                name = name,
                value = defaultconf.value)


        for rdl in release.releasedefaultlayer_set.all():
            try:
                lv =Layer_Version.objects.filter(layer__name = rdl.layer_name, up_branch__name = release.branch_name)[0].get_equivalents_wpriority(prj)[0]
                ProjectLayer.objects.create( project = prj,
                        layercommit = lv,
                        optional = False )
            except IndexError:
                # we may have no valid layer version objects, and that's ok
                pass

        return prj

    def create(self, *args, **kwargs):
        raise Exception("Invalid call to Project.objects.create. Use Project.objects.create_project() to create a project")

    def get_or_create(self, *args, **kwargs):
        raise Exception("Invalid call to Project.objects.get_or_create. Use Project.objects.create_project() to create a project")

class Project(models.Model):
    search_allowed_fields = ['name', 'short_description', 'release__name', 'release__branch_name']
    name = models.CharField(max_length=100)
    short_description = models.CharField(max_length=50, blank=True)
    bitbake_version = models.ForeignKey('BitbakeVersion')
    release     = models.ForeignKey("Release")
    created     = models.DateTimeField(auto_now_add = True)
    updated     = models.DateTimeField(auto_now = True)
    # This is a horrible hack; since Toaster has no "User" model available when
    # running in interactive mode, we can't reference the field here directly
    # Instead, we keep a possible null reference to the User id, as not to force
    # hard links to possibly missing models
    user_id     = models.IntegerField(null = True)
    objects     = ProjectManager()

    def __unicode__(self):
        return "%s (Release %s, BBV %s)" % (self.name, self.release, self.bitbake_version)

    def get_current_machine_name(self):
        try:
            return self.projectvariable_set.get(name="MACHINE").value
        except (ProjectVariable.DoesNotExist,IndexError):
            return( "None" );

    def get_number_of_builds(self):
        try:
            return len(Build.objects.filter( project = self.id ))
        except (Build.DoesNotExist,IndexError):
            return( 0 )

    def get_last_build_id(self):
        try:
            return Build.objects.filter( project = self.id ).order_by('-completed_on')[0].id
        except (Build.DoesNotExist,IndexError):
            return( -1 )

    def get_last_outcome(self):
        build_id = self.get_last_build_id
        if (-1 == build_id):
            return( "" )
        try:
            return Build.objects.filter( id = self.get_last_build_id )[ 0 ].outcome
        except (Build.DoesNotExist,IndexError):
            return( "not_found" )

    def get_last_target(self):
        build_id = self.get_last_build_id
        if (-1 == build_id):
            return( "" )
        try:
            return Target.objects.filter(build = build_id)[0].target
        except (Target.DoesNotExist,IndexError):
            return( "not_found" )

    def get_last_errors(self):
        build_id = self.get_last_build_id
        if (-1 == build_id):
            return( 0 )
        try:
            return Build.objects.filter(id = build_id)[ 0 ].errors_no
        except (Build.DoesNotExist,IndexError):
            return( "not_found" )

    def get_last_warnings(self):
        build_id = self.get_last_build_id
        if (-1 == build_id):
            return( 0 )
        try:
            return Build.objects.filter(id = build_id)[ 0 ].warnings_no
        except (Build.DoesNotExist,IndexError):
            return( "not_found" )

    def get_last_imgfiles(self):
        build_id = self.get_last_build_id
        if (-1 == build_id):
            return( "" )
        try:
            return Variable.objects.filter(build = build_id, variable_name = "IMAGE_FSTYPES")[ 0 ].variable_value
        except (Variable.DoesNotExist,IndexError):
            return( "not_found" )

    # returns a queryset of compatible layers for a project
    def compatible_layerversions(self, release = None, layer_name = None):
        if release == None:
            release = self.release
        # layers on the same branch or layers specifically set for this project
        queryset = Layer_Version.objects.filter((Q(up_branch__name = release.branch_name) & Q(project = None)) | Q(project = self) | Q(build__project = self))
        if layer_name is not None:
            # we select only a layer name
            queryset = queryset.filter(layer__name = layer_name)

        # order by layer version priority
        queryset = queryset.filter(layer_source__releaselayersourcepriority__release = release).order_by("-layer_source__releaselayersourcepriority__priority")

        return queryset

    # returns a set of layer-equivalent set of layers already in project
    def projectlayer_equivalent_set(self):
        return [j for i in [x.layercommit.get_equivalents_wpriority(self) for x in self.projectlayer_set.all().select_related("up_branch")] for j in i]

    def schedule_build(self):
        from bldcontrol.models import BuildRequest, BRTarget, BRLayer, BRVariable, BRBitbake
        br = BuildRequest.objects.create(project = self)
        try:

            BRBitbake.objects.create(req = br,
                giturl = self.bitbake_version.giturl,
                commit = self.bitbake_version.branch,
                dirpath = self.bitbake_version.dirpath)

            for l in self.projectlayer_set.all().order_by("pk"):
                commit = l.layercommit.get_vcs_reference()
                print("ii Building layer ", l.layercommit.layer.name, " at vcs point ", commit)
                BRLayer.objects.create(req = br, name = l.layercommit.layer.name, giturl = l.layercommit.layer.vcs_url, commit = commit, dirpath = l.layercommit.dirpath)
            for t in self.projecttarget_set.all():
                BRTarget.objects.create(req = br, target = t.target, task = t.task)
            for v in self.projectvariable_set.all():
                BRVariable.objects.create(req = br, name = v.name, value = v.value)

            br.state = BuildRequest.REQ_QUEUED
            br.save()
        except Exception as e:
            br.delete()
            raise e
        return br

class Build(models.Model):
    SUCCEEDED = 0
    FAILED = 1
    IN_PROGRESS = 2

    BUILD_OUTCOME = (
        (SUCCEEDED, 'Succeeded'),
        (FAILED, 'Failed'),
        (IN_PROGRESS, 'In Progress'),
    )

    search_allowed_fields = ['machine', 'cooker_log_path', "target__target", "target__target_image_file__file_name"]

    project = models.ForeignKey(Project, null = True)
    machine = models.CharField(max_length=100)
    distro = models.CharField(max_length=100)
    distro_version = models.CharField(max_length=100)
    started_on = models.DateTimeField()
    completed_on = models.DateTimeField()
    timespent = models.IntegerField(default=0)
    outcome = models.IntegerField(choices=BUILD_OUTCOME, default=IN_PROGRESS)
    errors_no = models.IntegerField(default=0)
    warnings_no = models.IntegerField(default=0)
    cooker_log_path = models.CharField(max_length=500)
    build_name = models.CharField(max_length=100)
    bitbake_version = models.CharField(max_length=50)

    def completeper(self):
        tf = Task.objects.filter(build = self)
        tfc = tf.count()
        if tfc > 0:
            completeper = tf.exclude(order__isnull=True).count()*100/tf.count()
        else:
            completeper = 0
        return completeper

    def eta(self):
        from django.utils import timezone
        eta = timezone.now()
        completeper = self.completeper()
        if self.completeper() > 0:
            eta += ((eta - self.started_on)*(100-completeper))/completeper
        return eta


    def get_sorted_target_list(self):
        tgts = Target.objects.filter(build_id = self.id).order_by( 'target' );
        return( tgts );

    @property
    def toaster_exceptions(self):
        return self.logmessage_set.filter(level=LogMessage.EXCEPTION)


# an Artifact is anything that results from a Build, and may be of interest to the user, and is not stored elsewhere
class BuildArtifact(models.Model):
    build = models.ForeignKey(Build)
    file_name = models.FilePathField()
    file_size = models.IntegerField()


    def get_local_file_name(self):
        try:
            deploydir = Variable.objects.get(build = self.build, variable_name="DEPLOY_DIR").variable_value
            return  self.file_name[len(deploydir)+1:]
        except:
            raise

        return self.file_name


    def is_available(self):
        if settings.MANAGED and build.project is not None:
            return build.buildrequest.environment.has_artifact(file_path)
        return False

class ProjectTarget(models.Model):
    project = models.ForeignKey(Project)
    target = models.CharField(max_length=100)
    task = models.CharField(max_length=100, null=True)

class Target(models.Model):
    search_allowed_fields = ['target', 'file_name']
    build = models.ForeignKey(Build)
    target = models.CharField(max_length=100)
    is_image = models.BooleanField(default = False)
    image_size = models.IntegerField(default=0)
    license_manifest_path = models.CharField(max_length=500, null=True)

    def package_count(self):
        return Target_Installed_Package.objects.filter(target_id__exact=self.id).count()

    def __unicode__(self):
        return self.target

class Target_Image_File(models.Model):
    target = models.ForeignKey(Target)
    file_name = models.FilePathField(max_length=254)
    file_size = models.IntegerField()

class Target_File(models.Model):
    ITYPE_REGULAR = 1
    ITYPE_DIRECTORY = 2
    ITYPE_SYMLINK = 3
    ITYPE_SOCKET = 4
    ITYPE_FIFO = 5
    ITYPE_CHARACTER = 6
    ITYPE_BLOCK = 7
    ITYPES = ( (ITYPE_REGULAR ,'regular'),
        ( ITYPE_DIRECTORY ,'directory'),
        ( ITYPE_SYMLINK ,'symlink'),
        ( ITYPE_SOCKET ,'socket'),
        ( ITYPE_FIFO ,'fifo'),
        ( ITYPE_CHARACTER ,'character'),
        ( ITYPE_BLOCK ,'block'),
        )

    target = models.ForeignKey(Target)
    path = models.FilePathField()
    size = models.IntegerField()
    inodetype = models.IntegerField(choices = ITYPES)
    permission = models.CharField(max_length=16)
    owner = models.CharField(max_length=128)
    group = models.CharField(max_length=128)
    directory = models.ForeignKey('Target_File', related_name="directory_set", null=True)
    sym_target = models.ForeignKey('Target_File', related_name="symlink_set", null=True)


class Task(models.Model):

    SSTATE_NA = 0
    SSTATE_MISS = 1
    SSTATE_FAILED = 2
    SSTATE_RESTORED = 3

    SSTATE_RESULT = (
        (SSTATE_NA, 'Not Applicable'), # For rest of tasks, but they still need checking.
        (SSTATE_MISS, 'File not in cache'), # the sstate object was not found
        (SSTATE_FAILED, 'Failed'), # there was a pkg, but the script failed
        (SSTATE_RESTORED, 'Succeeded'), # successfully restored
    )

    CODING_NA = 0
    CODING_PYTHON = 2
    CODING_SHELL = 3

    TASK_CODING = (
        (CODING_NA, 'N/A'),
        (CODING_PYTHON, 'Python'),
        (CODING_SHELL, 'Shell'),
    )

    OUTCOME_NA = -1
    OUTCOME_SUCCESS = 0
    OUTCOME_COVERED = 1
    OUTCOME_CACHED = 2
    OUTCOME_PREBUILT = 3
    OUTCOME_FAILED = 4
    OUTCOME_EMPTY = 5

    TASK_OUTCOME = (
        (OUTCOME_NA, 'Not Available'),
        (OUTCOME_SUCCESS, 'Succeeded'),
        (OUTCOME_COVERED, 'Covered'),
        (OUTCOME_CACHED, 'Cached'),
        (OUTCOME_PREBUILT, 'Prebuilt'),
        (OUTCOME_FAILED, 'Failed'),
        (OUTCOME_EMPTY, 'Empty'),
    )

    TASK_OUTCOME_HELP = (
        (OUTCOME_SUCCESS, 'This task successfully completed'),
        (OUTCOME_COVERED, 'This task did not run because its output is provided by another task'),
        (OUTCOME_CACHED, 'This task restored output from the sstate-cache directory or mirrors'),
        (OUTCOME_PREBUILT, 'This task did not run because its outcome was reused from a previous build'),
        (OUTCOME_FAILED, 'This task did not complete'),
        (OUTCOME_EMPTY, 'This task has no executable content'),
        (OUTCOME_NA, ''),
    )

    search_allowed_fields = [ "recipe__name", "recipe__version", "task_name", "logfile" ]

    def get_related_setscene(self):
        return Task.objects.filter(task_executed=True, build = self.build, recipe = self.recipe, task_name=self.task_name+"_setscene")

    def get_outcome_text(self):
        return Task.TASK_OUTCOME[self.outcome + 1][1]

    def get_outcome_help(self):
        return Task.TASK_OUTCOME_HELP[self.outcome][1]

    def get_sstate_text(self):
        if self.sstate_result==Task.SSTATE_NA:
            return ''
        else:
            return Task.SSTATE_RESULT[self.sstate_result][1]

    def get_executed_display(self):
        if self.task_executed:
            return "Executed"
        return "Not Executed"

    def get_description(self):
        if '_helptext' in vars(self) and self._helptext != None:
            return self._helptext
        try:
            self._helptext = HelpText.objects.get(key=self.task_name, area=HelpText.VARIABLE, build=self.build).text
        except HelpText.DoesNotExist:
            self._helptext = None

        return self._helptext

    build = models.ForeignKey(Build, related_name='task_build')
    order = models.IntegerField(null=True)
    task_executed = models.BooleanField(default=False) # True means Executed, False means Not/Executed
    outcome = models.IntegerField(choices=TASK_OUTCOME, default=OUTCOME_NA)
    sstate_checksum = models.CharField(max_length=100, blank=True)
    path_to_sstate_obj = models.FilePathField(max_length=500, blank=True)
    recipe = models.ForeignKey('Recipe', related_name='tasks')
    task_name = models.CharField(max_length=100)
    source_url = models.FilePathField(max_length=255, blank=True)
    work_directory = models.FilePathField(max_length=255, blank=True)
    script_type = models.IntegerField(choices=TASK_CODING, default=CODING_NA)
    line_number = models.IntegerField(default=0)
    disk_io = models.IntegerField(null=True)
    cpu_usage = models.DecimalField(max_digits=6, decimal_places=2, null=True)
    elapsed_time = models.DecimalField(max_digits=6, decimal_places=2, null=True)
    sstate_result = models.IntegerField(choices=SSTATE_RESULT, default=SSTATE_NA)
    message = models.CharField(max_length=240)
    logfile = models.FilePathField(max_length=255, blank=True)

    outcome_text = property(get_outcome_text)
    sstate_text  = property(get_sstate_text)

    def __unicode__(self):
        return "%d(%d) %s:%s" % (self.pk, self.build.pk, self.recipe.name, self.task_name)

    class Meta:
        ordering = ('order', 'recipe' ,)
        unique_together = ('build', 'recipe', 'task_name', )


class Task_Dependency(models.Model):
    task = models.ForeignKey(Task, related_name='task_dependencies_task')
    depends_on = models.ForeignKey(Task, related_name='task_dependencies_depends')

class Package(models.Model):
    search_allowed_fields = ['name', 'version', 'revision', 'recipe__name', 'recipe__version', 'recipe__license', 'recipe__layer_version__layer__name', 'recipe__layer_version__branch', 'recipe__layer_version__commit', 'recipe__layer_version__layer__local_path', 'installed_name']
    build = models.ForeignKey('Build')
    recipe = models.ForeignKey('Recipe', null=True)
    name = models.CharField(max_length=100)
    installed_name = models.CharField(max_length=100, default='')
    version = models.CharField(max_length=100, blank=True)
    revision = models.CharField(max_length=32, blank=True)
    summary = models.TextField(blank=True)
    description = models.TextField(blank=True)
    size = models.IntegerField(default=0)
    installed_size = models.IntegerField(default=0)
    section = models.CharField(max_length=80, blank=True)
    license = models.CharField(max_length=80, blank=True)

class Package_DependencyManager(models.Manager):
    use_for_related_fields = True

    def get_query_set(self):
        return super(Package_DependencyManager, self).get_query_set().exclude(package_id = F('depends_on__id'))

class Package_Dependency(models.Model):
    TYPE_RDEPENDS = 0
    TYPE_TRDEPENDS = 1
    TYPE_RRECOMMENDS = 2
    TYPE_TRECOMMENDS = 3
    TYPE_RSUGGESTS = 4
    TYPE_RPROVIDES = 5
    TYPE_RREPLACES = 6
    TYPE_RCONFLICTS = 7
    ' TODO: bpackage should be changed to remove the DEPENDS_TYPE access '
    DEPENDS_TYPE = (
        (TYPE_RDEPENDS, "depends"),
        (TYPE_TRDEPENDS, "depends"),
        (TYPE_TRECOMMENDS, "recommends"),
        (TYPE_RRECOMMENDS, "recommends"),
        (TYPE_RSUGGESTS, "suggests"),
        (TYPE_RPROVIDES, "provides"),
        (TYPE_RREPLACES, "replaces"),
        (TYPE_RCONFLICTS, "conflicts"),
    )
    """ Indexed by dep_type, in view order, key for short name and help
        description which when viewed will be printf'd with the
        package name.
    """
    DEPENDS_DICT = {
        TYPE_RDEPENDS :     ("depends", "%s is required to run %s"),
        TYPE_TRDEPENDS :    ("depends", "%s is required to run %s"),
        TYPE_TRECOMMENDS :  ("recommends", "%s extends the usability of %s"),
        TYPE_RRECOMMENDS :  ("recommends", "%s extends the usability of %s"),
        TYPE_RSUGGESTS :    ("suggests", "%s is suggested for installation with %s"),
        TYPE_RPROVIDES :    ("provides", "%s is provided by %s"),
        TYPE_RREPLACES :    ("replaces", "%s is replaced by %s"),
        TYPE_RCONFLICTS :   ("conflicts", "%s conflicts with %s, which will not be installed if this package is not first removed"),
    }

    package = models.ForeignKey(Package, related_name='package_dependencies_source')
    depends_on = models.ForeignKey(Package, related_name='package_dependencies_target')   # soft dependency
    dep_type = models.IntegerField(choices=DEPENDS_TYPE)
    target = models.ForeignKey(Target, null=True)
    objects = Package_DependencyManager()

class Target_Installed_Package(models.Model):
    target = models.ForeignKey(Target)
    package = models.ForeignKey(Package, related_name='buildtargetlist_package')

class Package_File(models.Model):
    package = models.ForeignKey(Package, related_name='buildfilelist_package')
    path = models.FilePathField(max_length=255, blank=True)
    size = models.IntegerField()

class Recipe(models.Model):
    search_allowed_fields = ['name', 'version', 'file_path', 'section', 'summary', 'description', 'license', 'layer_version__layer__name', 'layer_version__branch', 'layer_version__commit', 'layer_version__layer__local_path', 'layer_version__layer_source__name']

    layer_source = models.ForeignKey('LayerSource', default = None, null = True)  # from where did we get this recipe
    up_id = models.IntegerField(null = True, default = None)                    # id of entry in the source
    up_date = models.DateTimeField(null = True, default = None)

    name = models.CharField(max_length=100, blank=True)                 # pn
    version = models.CharField(max_length=100, blank=True)              # pv
    layer_version = models.ForeignKey('Layer_Version', related_name='recipe_layer_version')
    summary = models.TextField(blank=True)
    description = models.TextField(blank=True)
    section = models.CharField(max_length=100, blank=True)
    license = models.CharField(max_length=200, blank=True)
    homepage = models.URLField(blank=True)
    bugtracker = models.URLField(blank=True)
    file_path = models.FilePathField(max_length=255)

    def get_layersource_view_url(self):
        if self.layer_source is None:
            return ""

        url = self.layer_source.get_object_view(self.layer_version.up_branch, "recipes", self.name)
        return url

    def __unicode__(self):
        return "Recipe " + self.name + ":" + self.version

    def get_local_path(self):
        if settings.MANAGED and self.layer_version.build.project is not None:
            return self.file_path[len(self.layer_version.layer.local_path)+1:]

        return self.file_path

    class Meta:
        unique_together = ("layer_version", "file_path")

class Recipe_DependencyManager(models.Manager):
    use_for_related_fields = True

    def get_query_set(self):
        return super(Recipe_DependencyManager, self).get_query_set().exclude(recipe_id = F('depends_on__id'))

class Recipe_Dependency(models.Model):
    TYPE_DEPENDS = 0
    TYPE_RDEPENDS = 1

    DEPENDS_TYPE = (
        (TYPE_DEPENDS, "depends"),
        (TYPE_RDEPENDS, "rdepends"),
    )
    recipe = models.ForeignKey(Recipe, related_name='r_dependencies_recipe')
    depends_on = models.ForeignKey(Recipe, related_name='r_dependencies_depends')
    dep_type = models.IntegerField(choices=DEPENDS_TYPE)
    objects = Recipe_DependencyManager()


class Machine(models.Model):
    search_allowed_fields = ["name", "description", "layer_version__layer__name"]
    layer_source = models.ForeignKey('LayerSource', default = None, null = True)  # from where did we get this machine
    up_id = models.IntegerField(null = True, default = None)                      # id of entry in the source
    up_date = models.DateTimeField(null = True, default = None)

    layer_version = models.ForeignKey('Layer_Version')
    name = models.CharField(max_length=255)
    description = models.CharField(max_length=255)

    def get_vcs_machine_file_link_url(self):
        path = 'conf/machine/'+self.name+'.conf'

        return self.layer_version.get_vcs_file_link_url(path)

    def __unicode__(self):
        return "Machine " + self.name + "(" + self.description + ")"

    class Meta:
        unique_together = ("layer_source", "up_id")


from django.db.models.base import ModelBase

class InheritanceMetaclass(ModelBase):
    def __call__(cls, *args, **kwargs):
        obj = super(InheritanceMetaclass, cls).__call__(*args, **kwargs)
        return obj.get_object()


class LayerSource(models.Model):
    __metaclass__ = InheritanceMetaclass

    class Meta:
        unique_together = (('sourcetype', 'apiurl'), )

    TYPE_LOCAL = 0
    TYPE_LAYERINDEX = 1
    TYPE_IMPORTED = 2
    SOURCE_TYPE = (
        (TYPE_LOCAL, "local"),
        (TYPE_LAYERINDEX, "layerindex"),
        (TYPE_IMPORTED, "imported"),
      )

    name = models.CharField(max_length=63, unique = True)
    sourcetype = models.IntegerField(choices=SOURCE_TYPE)
    apiurl = models.CharField(max_length=255, null=True, default=None)

    def update(self):
        """
            Updates the local database information from the upstream layer source
        """
        raise Exception("Abstract, update() must be implemented by all LayerSource-derived classes (object is %s)" % str(vars(self)))

    def save(self, *args, **kwargs):
        if isinstance(self, LocalLayerSource):
            self.sourcetype = LayerSource.TYPE_LOCAL
        elif isinstance(self, LayerIndexLayerSource):
            self.sourcetype = LayerSource.TYPE_LAYERINDEX
        elif isinstance(self, ImportedLayerSource):
            self.sourcetype = LayerSource.TYPE_IMPORTED
        elif self.sourcetype == None:
            raise Exception("Unknown LayerSource-derived class. If you added a new layer source type, fill out all code stubs.")
        return super(LayerSource, self).save(*args, **kwargs)

    def get_object(self):
        if self.sourcetype == LayerSource.TYPE_LOCAL:
            self.__class__ = LocalLayerSource
        elif self.sourcetype == LayerSource.TYPE_LAYERINDEX:
            self.__class__ = LayerIndexLayerSource
        elif self.sourcetype == LayerSource.TYPE_IMPORTED:
            self.__class__ = ImportedLayerSource
        else:
            raise Exception("Unknown LayerSource type. If you added a new layer source type, fill out all code stubs.")
        return self

    def __unicode__(self):
        return "%s (%s)" % (self.name, self.sourcetype)


class LocalLayerSource(LayerSource):
    class Meta(LayerSource._meta.__class__):
        proxy = True

    def __init__(self, *args, **kwargs):
        super(LocalLayerSource, self).__init__(args, kwargs)
        self.sourcetype = LayerSource.TYPE_LOCAL

    def update(self):
        """
            Fetches layer, recipe and machine information from local repository
        """
        pass

class ImportedLayerSource(LayerSource):
    class Meta(LayerSource._meta.__class__):
        proxy = True

    def __init__(self, *args, **kwargs):
        super(ImportedLayerSource, self).__init__(args, kwargs)
        self.sourcetype = LayerSource.TYPE_IMPORTED

    def update(self):
        """
            Fetches layer, recipe and machine information from local repository
        """
        pass


class LayerIndexLayerSource(LayerSource):
    class Meta(LayerSource._meta.__class__):
        proxy = True

    def __init__(self, *args, **kwargs):
        super(LayerIndexLayerSource, self).__init__(args, kwargs)
        self.sourcetype = LayerSource.TYPE_LAYERINDEX

    def get_object_view(self, branch, objectype, upid):
        return self.apiurl + "../branch/" + branch.name + "/" + objectype + "/?q=" + str(upid)

    def update(self):
        """
            Fetches layer, recipe and machine information from remote repository
        """
        assert self.apiurl is not None
        from django.db import IntegrityError
        from django.db import transaction, connection

        import httplib, urlparse, json
        import os
        proxy_settings = os.environ.get("http_proxy", None)

        def _get_json_response(apiurl = self.apiurl):
            conn = None
            _parsedurl = urlparse.urlparse(apiurl)
            path = _parsedurl.path
            query = _parsedurl.query
            def parse_url(url):
                parsedurl = urlparse.urlparse(url)
                try:
                    (host, port) = parsedurl.netloc.split(":")
                except ValueError:
                    host = parsedurl.netloc
                    port = None

                if port is None:
                    port = 80
                else:
                    port = int(port)
                return (host, port)

            if proxy_settings is None:
                host, port = parse_url(apiurl)
                conn = httplib.HTTPConnection(host, port)
                conn.request("GET", path + "?" + query)
            else:
                host, port = parse_url(proxy_settings)
                conn = httplib.HTTPConnection(host, port)
                conn.request("GET", apiurl)

            r = conn.getresponse()
            if r.status != 200:
                raise Exception("Failed to read " + path + ": %d %s" % (r.status, r.reason))
            return json.loads(r.read())

        # verify we can get the basic api
        try:
            apilinks = _get_json_response()
        except Exception as e:
            import traceback
            if proxy_settings is not None:
                print "EE: Using proxy ", proxy_settings
            print "EE: could not connect to %s, skipping update: %s\n%s" % (self.apiurl, e, traceback.format_exc(e))
            return

        # update branches; only those that we already have names listed in the Releases table
        whitelist_branch_names = map(lambda x: x.branch_name, Release.objects.all())

        print "Fetching branches"
        branches_info = _get_json_response(apilinks['branches']
            + "?filter=name:%s" % "OR".join(whitelist_branch_names))
        for bi in branches_info:
            b, created = Branch.objects.get_or_create(layer_source = self, name = bi['name'])
            b.up_id = bi['id']
            b.up_date = bi['updated']
            b.name = bi['name']
            b.short_description = bi['short_description']
            b.save()

        # update layers
        layers_info = _get_json_response(apilinks['layerItems'])
        if not connection.features.autocommits_when_autocommit_is_off:
            transaction.set_autocommit(False)
        for li in layers_info:
            l, created = Layer.objects.get_or_create(layer_source = self, name = li['name'])
            l.up_id = li['id']
            l.up_date = li['updated']
            l.vcs_url = li['vcs_url']
            l.vcs_web_url = li['vcs_web_url']
            l.vcs_web_tree_base_url = li['vcs_web_tree_base_url']
            l.vcs_web_file_base_url = li['vcs_web_file_base_url']
            l.summary = li['summary']
            l.description = li['description']
            l.save()
        if not connection.features.autocommits_when_autocommit_is_off:
            transaction.set_autocommit(True)

        # update layerbranches/layer_versions
        print "Fetching layer information"
        layerbranches_info = _get_json_response(apilinks['layerBranches']
                + "?filter=branch:%s" % "OR".join(map(lambda x: str(x.up_id), [i for i in Branch.objects.filter(layer_source = self) if i.up_id is not None] ))
            )

        if not connection.features.autocommits_when_autocommit_is_off:
            transaction.set_autocommit(False)
        for lbi in layerbranches_info:
            lv, created = Layer_Version.objects.get_or_create(layer_source = self,
                    up_id = lbi['id'],
                    layer=Layer.objects.get(layer_source = self, up_id = lbi['layer'])
                )

            lv.up_date = lbi['updated']
            lv.up_branch = Branch.objects.get(layer_source = self, up_id = lbi['branch'])
            lv.branch = lbi['actual_branch']
            lv.commit = lbi['actual_branch']
            lv.dirpath = lbi['vcs_subdir']
            lv.save()
        if not connection.features.autocommits_when_autocommit_is_off:
            transaction.set_autocommit(True)

        # update layer dependencies
        layerdependencies_info = _get_json_response(apilinks['layerDependencies'])
        dependlist = {}
        if not connection.features.autocommits_when_autocommit_is_off:
            transaction.set_autocommit(False)
        for ldi in layerdependencies_info:
            try:
                lv = Layer_Version.objects.get(layer_source = self, up_id = ldi['layerbranch'])
            except Layer_Version.DoesNotExist as e:
                continue

            if lv not in dependlist:
                dependlist[lv] = []
            try:
                dependlist[lv].append(Layer_Version.objects.get(layer_source = self, layer__up_id = ldi['dependency'], up_branch = lv.up_branch))
            except Layer_Version.DoesNotExist as e:
                print "Cannot find layer version ", self, ldi['dependency'], lv.up_branch
                raise e

        for lv in dependlist:
            LayerVersionDependency.objects.filter(layer_version = lv).delete()
            for lvd in dependlist[lv]:
                LayerVersionDependency.objects.get_or_create(layer_version = lv, depends_on = lvd)
        if not connection.features.autocommits_when_autocommit_is_off:
            transaction.set_autocommit(True)


        # update machines
        print "Fetching machine information"
        machines_info = _get_json_response(apilinks['machines']
                + "?filter=layerbranch:%s" % "OR".join(map(lambda x: str(x.up_id), Layer_Version.objects.filter(layer_source = self)))
            )

        if not connection.features.autocommits_when_autocommit_is_off:
            transaction.set_autocommit(False)
        for mi in machines_info:
            mo, created = Machine.objects.get_or_create(layer_source = self, up_id = mi['id'], layer_version = Layer_Version.objects.get(layer_source = self, up_id = mi['layerbranch']))
            mo.up_date = mi['updated']
            mo.name = mi['name']
            mo.description = mi['description']
            mo.save()

        if not connection.features.autocommits_when_autocommit_is_off:
            transaction.set_autocommit(True)

        # update recipes; paginate by layer version / layer branch
        print "Fetching target information"
        recipes_info = _get_json_response(apilinks['recipes']
                + "?filter=layerbranch:%s" % "OR".join(map(lambda x: str(x.up_id), Layer_Version.objects.filter(layer_source = self)))
            )
        if not connection.features.autocommits_when_autocommit_is_off:
            transaction.set_autocommit(False)
        for ri in recipes_info:
            try:
                ro, created = Recipe.objects.get_or_create(layer_source = self, up_id = ri['id'], layer_version = Layer_Version.objects.get(layer_source = self, up_id = ri['layerbranch']))
                ro.up_date = ri['updated']
                ro.name = ri['pn']
                ro.version = ri['pv']
                ro.summary = ri['summary']
                ro.description = ri['description']
                ro.section = ri['section']
                ro.license = ri['license']
                ro.homepage = ri['homepage']
                ro.bugtracker = ri['bugtracker']
                ro.file_path = ri['filepath'] + "/" + ri['filename']
                ro.save()
            except:
                #print "Duplicate Recipe, ignoring: ", vars(ro)
                pass
        if not connection.features.autocommits_when_autocommit_is_off:
            transaction.set_autocommit(True)
        pass

class BitbakeVersion(models.Model):

    name = models.CharField(max_length=32, unique = True)
    giturl = GitURLField()
    branch = models.CharField(max_length=32)
    dirpath = models.CharField(max_length=255)

    def __unicode__(self):
        return "%s (Branch: %s)" % (self.name, self.branch)


class Release(models.Model):
    """ A release is a project template, used to pre-populate Project settings with a configuration set """
    name = models.CharField(max_length=32, unique = True)
    description = models.CharField(max_length=255)
    bitbake_version = models.ForeignKey(BitbakeVersion)
    branch_name = models.CharField(max_length=50, default = "")
    helptext = models.TextField(null=True)

    def __unicode__(self):
        return "%s (%s)" % (self.name, self.branch_name)

class ReleaseLayerSourcePriority(models.Model):
    """ Each release selects layers from the set up layer sources, ordered by priority """
    release = models.ForeignKey("Release")
    layer_source = models.ForeignKey("LayerSource")
    priority = models.IntegerField(default = 0)

    def __unicode__(self):
        return "%s-%s:%d" % (self.release.name, self.layer_source.name, self.priority)
    class Meta:
        unique_together = (('release', 'layer_source'),)


class ReleaseDefaultLayer(models.Model):
    release = models.ForeignKey(Release)
    layer_name = models.CharField(max_length=100, default="")


# Branch class is synced with layerindex.Branch, branches can only come from remote layer indexes
class Branch(models.Model):
    layer_source = models.ForeignKey('LayerSource', null = True, default = True)
    up_id = models.IntegerField(null = True, default = None)                    # id of branch in the source
    up_date = models.DateTimeField(null = True, default = None)

    name = models.CharField(max_length=50)
    short_description = models.CharField(max_length=50, blank=True)

    class Meta:
        verbose_name_plural = "Branches"
        unique_together = (('layer_source', 'name'),('layer_source', 'up_id'))

    def __unicode__(self):
        return self.name


# Layer class synced with layerindex.LayerItem
class Layer(models.Model):
    layer_source = models.ForeignKey(LayerSource, null = True, default = None)  # from where did we got this layer
    up_id = models.IntegerField(null = True, default = None)                    # id of layer in the remote source
    up_date = models.DateTimeField(null = True, default = None)

    name = models.CharField(max_length=100)
    local_path = models.FilePathField(max_length=255, null = True, default = None)
    layer_index_url = models.URLField()
    vcs_url = GitURLField(default = None, null = True)
    vcs_web_url = models.URLField(null = True, default = None)
    vcs_web_tree_base_url = models.URLField(null = True, default = None)
    vcs_web_file_base_url = models.URLField(null = True, default = None)

    summary = models.TextField(help_text='One-line description of the layer', null = True, default = None)
    description = models.TextField(null = True, default = None)

    def __unicode__(self):
        return "%s / %s " % (self.name, self.layer_source)

    class Meta:
        unique_together = (("layer_source", "up_id"), ("layer_source", "name"))


# LayerCommit class is synced with layerindex.LayerBranch
class Layer_Version(models.Model):
    search_allowed_fields = ["layer__name", "layer__summary", "layer__description", "layer__vcs_url", "dirpath", "up_branch__name", "commit", "branch"]
    build = models.ForeignKey(Build, related_name='layer_version_build', default = None, null = True)
    layer = models.ForeignKey(Layer, related_name='layer_version_layer')

    layer_source = models.ForeignKey(LayerSource, null = True, default = None)                   # from where did we get this Layer Version
    up_id = models.IntegerField(null = True, default = None)        # id of layerbranch in the remote source
    up_date = models.DateTimeField(null = True, default = None)
    up_branch = models.ForeignKey(Branch, null = True, default = None)

    branch = models.CharField(max_length=80)            # LayerBranch.actual_branch
    commit = models.CharField(max_length=100)           # LayerBranch.vcs_last_rev
    dirpath = models.CharField(max_length=255, null = True, default = None)          # LayerBranch.vcs_subdir
    priority = models.IntegerField(default = 0)         # if -1, this is a default layer

    project = models.ForeignKey('Project', null = True, default = None)   # Set if this layer is project-specific; always set for imported layers, and project-set branches

    # code lifted, with adaptations, from the layerindex-web application https://git.yoctoproject.org/cgit/cgit.cgi/layerindex-web/
    def _handle_url_path(self, base_url, path):
        import re, posixpath
        if base_url:
            if self.dirpath:
                if path:
                    extra_path = self.dirpath + '/' + path
                    # Normalise out ../ in path for usage URL
                    extra_path = posixpath.normpath(extra_path)
                    # Minor workaround to handle case where subdirectory has been added between branches
                    # (should probably support usage URL per branch to handle this... sigh...)
                    if extra_path.startswith('../'):
                        extra_path = extra_path[3:]
                else:
                    extra_path = self.dirpath
            else:
                extra_path = path
            branchname = self.up_branch.name
            url = base_url.replace('%branch%', branchname)

            # If there's a % in the path (e.g. a wildcard bbappend) we need to encode it
            if extra_path:
                extra_path = extra_path.replace('%', '%25')

            if '%path%' in base_url:
                if extra_path:
                    url = re.sub(r'\[([^\]]*%path%[^\]]*)\]', '\\1', url)
                else:
                    url = re.sub(r'\[([^\]]*%path%[^\]]*)\]', '', url)
                return url.replace('%path%', extra_path)
            else:
                return url + extra_path
        return None

    def get_vcs_link_url(self):
        if self.layer.vcs_web_url is None:
            return None
        return self.layer.vcs_web_url

    def get_vcs_file_link_url(self, file_path=""):
        if self.layer.vcs_web_file_base_url is None:
            return None
        return self._handle_url_path(self.layer.vcs_web_file_base_url, file_path)

    def get_vcs_dirpath_link_url(self):
        if self.layer.vcs_web_tree_base_url is None:
            return None
        return self._handle_url_path(self.layer.vcs_web_tree_base_url, '')

    def get_equivalents_wpriority(self, project):
        """ Returns an ordered layerversion list that satisfies a LayerVersionDependency using the layer name and the current Project Releases' LayerSource priority """

        # layers created for this project, or coming from a build inthe project
        query = Q(project = project) | Q(build__project = project)
        if self.up_branch is not None:
            # the same up_branch name
            query |= Q(up_branch__name=self.up_branch.name)
        else:
            # or we have a layer in the project that's similar to mine (See the layer.name constraint below)
            query |= Q(projectlayer__project=project)

        candidate_layer_versions = list(Layer_Version.objects.filter(layer__name = self.layer.name).filter(query).select_related('layer_source', 'layer', 'up_branch').order_by("-id"))

        # optimization - if we have only one, we don't need no stinking sort
        if len(candidate_layer_versions) == 1:
            return candidate_layer_versions

#        raise Exception(candidate_layer_versions)

        release_priorities = {}

        for ls_id, prio in map(lambda x: (x.layer_source_id, x.priority), project.release.releaselayersourcepriority_set.all().order_by("-priority")):
            release_priorities[ls_id] = prio

        def _get_ls_priority(ls):
            # if there is no layer source, we have minus infinite priority, as we don't want this layer selected
            if ls == None:
                return -10000
            try:
                return release_priorities[ls.id]
            except IndexError:
                raise Exception("Unknown %d %s" % (ls.id, release_priorities))

        return sorted( candidate_layer_versions ,
                key = lambda x: _get_ls_priority(x.layer_source),
                reverse = True)

    def get_vcs_reference(self):
        if self.commit is not None and len(self.commit) > 0:
            return self.commit
        if self.branch is not None and len(self.branch) > 0:
            return self.branch
        if self.up_branch is not None:
            return self.up_branch.name
        return ("Cannot determine the vcs_reference for layer version %s" % vars(self))

    def __unicode__(self):
        return "%d %s (VCS %s, Project %s)" % (self.pk, str(self.layer), self.get_vcs_reference(), self.build.project if self.build is not None else "No project")

    class Meta:
        unique_together = ("layer_source", "up_id")

class LayerVersionDependency(models.Model):
    layer_source = models.ForeignKey(LayerSource, null = True, default = None)  # from where did we got this layer
    up_id = models.IntegerField(null = True, default = None)                    # id of layerbranch in the remote source

    layer_version = models.ForeignKey(Layer_Version, related_name="dependencies")
    depends_on = models.ForeignKey(Layer_Version, related_name="dependees")

    class Meta:
        unique_together = ("layer_source", "up_id")

class ProjectLayer(models.Model):
    project = models.ForeignKey(Project)
    layercommit = models.ForeignKey(Layer_Version, null=True)
    optional = models.BooleanField(default = True)

    def __unicode__(self):
        return "%s, %s" % (self.project.name, self.layercommit)

    class Meta:
        unique_together = (("project", "layercommit"),)

class ProjectVariable(models.Model):
    project = models.ForeignKey(Project)
    name = models.CharField(max_length=100)
    value = models.TextField(blank = True)

class Variable(models.Model):
    search_allowed_fields = ['variable_name', 'variable_value',
                             'vhistory__file_name', "description"]
    build = models.ForeignKey(Build, related_name='variable_build')
    variable_name = models.CharField(max_length=100)
    variable_value = models.TextField(blank=True)
    changed = models.BooleanField(default=False)
    human_readable_name = models.CharField(max_length=200)
    description = models.TextField(blank=True)

class VariableHistory(models.Model):
    variable = models.ForeignKey(Variable, related_name='vhistory')
    value   = models.TextField(blank=True)
    file_name = models.FilePathField(max_length=255)
    line_number = models.IntegerField(null=True)
    operation = models.CharField(max_length=64)

class HelpText(models.Model):
    VARIABLE = 0
    HELPTEXT_AREA = ((VARIABLE, 'variable'), )

    build = models.ForeignKey(Build, related_name='helptext_build')
    area = models.IntegerField(choices=HELPTEXT_AREA)
    key = models.CharField(max_length=100)
    text = models.TextField()

class LogMessage(models.Model):
    EXCEPTION = -1      # used to signal self-toaster-exceptions
    INFO = 0
    WARNING = 1
    ERROR = 2

    LOG_LEVEL = ( (INFO, "info"),
            (WARNING, "warn"),
            (ERROR, "error"),
            (EXCEPTION, "toaster exception"))

    build = models.ForeignKey(Build)
    task  = models.ForeignKey(Task, blank = True, null=True)
    level = models.IntegerField(choices=LOG_LEVEL, default=INFO)
    message=models.CharField(max_length=240)
    pathname = models.FilePathField(max_length=255, blank=True)
    lineno = models.IntegerField(null=True)
