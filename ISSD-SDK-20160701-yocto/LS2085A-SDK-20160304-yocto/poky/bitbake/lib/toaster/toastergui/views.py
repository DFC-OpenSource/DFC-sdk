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

import operator,re
import HTMLParser

from django.db.models import Q, Sum, Count, Max
from django.db import IntegrityError
from django.shortcuts import render, redirect
from orm.models import Build, Target, Task, Layer, Layer_Version, Recipe, LogMessage, Variable
from orm.models import Task_Dependency, Recipe_Dependency, Package, Package_File, Package_Dependency
from orm.models import Target_Installed_Package, Target_File, Target_Image_File, BuildArtifact
from django.views.decorators.cache import cache_control
from django.core.urlresolvers import reverse
from django.core.exceptions import MultipleObjectsReturned
from django.core.paginator import Paginator, EmptyPage, PageNotAnInteger
from django.http import HttpResponseBadRequest, HttpResponseNotFound
from django.utils import timezone
from django.utils.html import escape
from datetime import timedelta, datetime, date
from django.utils import formats
from toastergui.templatetags.projecttags import json as jsonfilter
import json
import mimetypes

class MimeTypeFinder(object):
    # setting this to False enables additional non-standard mimetypes
    # to be included in the guess
    _strict = False

    # returns the mimetype for a file path as a string,
    # or 'application/octet-stream' if the type couldn't be guessed
    @classmethod
    def get_mimetype(self, path):
        guess = mimetypes.guess_type(path, self._strict)
        guessed_type = guess[0]
        if guessed_type == None:
            guessed_type = 'application/octet-stream'
        return guessed_type

# all new sessions should come through the landing page;
# determine in which mode we are running in, and redirect appropriately
def landing(request):
    if toastermain.settings.MANAGED:
        from bldcontrol.models import BuildRequest
        if BuildRequest.objects.count() == 0 and Project.objects.count() > 0:
            return redirect(reverse('all-projects'), permanent = False)

        if BuildRequest.objects.all().count() > 0:
            return redirect(reverse('all-builds'), permanent = False)
    else:
        if Build.objects.all().count() > 0:
            return redirect(reverse('all-builds'), permanent = False)

    context = {}
    if toastermain.settings.MANAGED:
        context['lvs_nos'] = Layer_Version.objects.all().count()

    return render(request, 'landing.html', context)

# returns a list for most recent builds; for use in the Project page, xhr_ updates,  and other places, as needed
def _project_recent_build_list(prj):
    return map(lambda x: {
            "id":  x.pk,
            "targets" : map(lambda y: {"target": y.target, "task": y.task }, x.brtarget_set.all()),
            "status": x.get_state_display(),
            "errors": map(lambda y: {"type": y.errtype, "msg": y.errmsg, "tb": y.traceback}, x.brerror_set.all()),
            "updated": x.updated.strftime('%s')+"000",
            "command_time": (x.updated - x.created).total_seconds(),
            "br_page_url": reverse('buildrequestdetails', args=(x.project.id, x.pk) ),
            "build" : map( lambda y: {"id": y.pk,
                        "status": y.get_outcome_display(),
                        "completed_on" : y.completed_on.strftime('%s')+"000",
                        "build_time" : (y.completed_on - y.started_on).total_seconds(),
                        "build_page_url" : reverse('builddashboard', args=(y.pk,)),
                        'build_time_page_url': reverse('buildtime', args=(y.pk,)),
                        "errors": y.errors_no,
                        "warnings": y.warnings_no,
                        "completeper": y.completeper() if y.outcome == Build.IN_PROGRESS else "0",
                        "eta": y.eta().strftime('%s')+"000" if y.outcome == Build.IN_PROGRESS else "0",
                        }, Build.objects.filter(buildrequest = x)),
        }, list(prj.buildrequest_set.filter(Q(state__lt=BuildRequest.REQ_COMPLETED) or Q(state=BuildRequest.REQ_DELETED)).order_by("-pk")) +
            list(prj.buildrequest_set.filter(state__in=[BuildRequest.REQ_COMPLETED, BuildRequest.REQ_FAILED]).order_by("-pk")[:3]))


def _build_page_range(paginator, index = 1):
    try:
        page = paginator.page(index)
    except PageNotAnInteger:
        page = paginator.page(1)
    except  EmptyPage:
        page = paginator.page(paginator.num_pages)


    page.page_range = [page.number]
    crt_range = 0
    for i in range(1,5):
        if (page.number + i) <= paginator.num_pages:
            page.page_range = page.page_range + [ page.number + i]
            crt_range +=1
        if (page.number - i) > 0:
            page.page_range =  [page.number -i] + page.page_range
            crt_range +=1
        if crt_range == 4:
            break
    return page


def _verify_parameters(g, mandatory_parameters):
    miss = []
    for mp in mandatory_parameters:
        if not mp in g:
            miss.append(mp)
    if len(miss):
        return miss
    return None

def _redirect_parameters(view, g, mandatory_parameters, *args, **kwargs):
    import urllib
    url = reverse(view, kwargs=kwargs)
    params = {}
    for i in g:
        params[i] = g[i]
    for i in mandatory_parameters:
        if not i in params:
            params[i] = urllib.unquote(str(mandatory_parameters[i]))

    return redirect(url + "?%s" % urllib.urlencode(params), *args, **kwargs)

FIELD_SEPARATOR = ":"
AND_VALUE_SEPARATOR = "!"
OR_VALUE_SEPARATOR = "|"
DESCENDING = "-"

def __get_q_for_val(name, value):
    if "OR" in value:
        return reduce(operator.or_, map(lambda x: __get_q_for_val(name, x), [ x for x in value.split("OR") ]))
    if "AND" in value:
        return reduce(operator.and_, map(lambda x: __get_q_for_val(name, x), [ x for x in value.split("AND") ]))
    if value.startswith("NOT"):
        value = value[3:]
        if value == 'None':
            value = None
        kwargs = { name : value }
        return ~Q(**kwargs)
    else:
        if value == 'None':
            value = None
        kwargs = { name : value }
        return Q(**kwargs)

def _get_filtering_query(filter_string):

    search_terms = filter_string.split(FIELD_SEPARATOR)
    and_keys = search_terms[0].split(AND_VALUE_SEPARATOR)
    and_values = search_terms[1].split(AND_VALUE_SEPARATOR)

    and_query = []
    for kv in zip(and_keys, and_values):
        or_keys = kv[0].split(OR_VALUE_SEPARATOR)
        or_values = kv[1].split(OR_VALUE_SEPARATOR)
        querydict = dict(zip(or_keys, or_values))
        and_query.append(reduce(operator.or_, map(lambda x: __get_q_for_val(x, querydict[x]), [k for k in querydict])))

    return reduce(operator.and_, [k for k in and_query])

def _get_toggle_order(request, orderkey, reverse = False):
    if reverse:
        return "%s:+" % orderkey if request.GET.get('orderby', "") == "%s:-" % orderkey else "%s:-" % orderkey
    else:
        return "%s:-" % orderkey if request.GET.get('orderby', "") == "%s:+" % orderkey else "%s:+" % orderkey

def _get_toggle_order_icon(request, orderkey):
    if request.GET.get('orderby', "") == "%s:+"%orderkey:
        return "down"
    elif request.GET.get('orderby', "") == "%s:-"%orderkey:
        return "up"
    else:
        return None

# we check that the input comes in a valid form that we can recognize
def _validate_input(input, model):

    invalid = None

    if input:
        input_list = input.split(FIELD_SEPARATOR)

        # Check we have only one colon
        if len(input_list) != 2:
            invalid = "We have an invalid number of separators: " + input + " -> " + str(input_list)
            return None, invalid

        # Check we have an equal number of terms both sides of the colon
        if len(input_list[0].split(AND_VALUE_SEPARATOR)) != len(input_list[1].split(AND_VALUE_SEPARATOR)):
            invalid = "Not all arg names got values"
            return None, invalid + str(input_list)

        # Check we are looking for a valid field
        valid_fields = model._meta.get_all_field_names()
        for field in input_list[0].split(AND_VALUE_SEPARATOR):
            if not reduce(lambda x, y: x or y, map(lambda x: field.startswith(x), [ x for x in valid_fields ])):
                return None, (field, [ x for x in valid_fields ])

    return input, invalid

# uses search_allowed_fields in orm/models.py to create a search query
# for these fields with the supplied input text
def _get_search_results(search_term, queryset, model):
    search_objects = []
    for st in search_term.split(" "):
        q_map = map(lambda x: Q(**{x+'__icontains': st}),
                model.search_allowed_fields)

        search_objects.append(reduce(operator.or_, q_map))
    search_object = reduce(operator.and_, search_objects)
    queryset = queryset.filter(search_object)

    return queryset


# function to extract the search/filter/ordering parameters from the request
# it uses the request and the model to validate input for the filter and orderby values
def _search_tuple(request, model):
    ordering_string, invalid = _validate_input(request.GET.get('orderby', ''), model)
    if invalid:
        raise BaseException("Invalid ordering model:" + str(model) + str(invalid))

    filter_string, invalid = _validate_input(request.GET.get('filter', ''), model)
    if invalid:
        raise BaseException("Invalid filter " + str(invalid))

    search_term = request.GET.get('search', '')
    return (filter_string, search_term, ordering_string)


# returns a lazy-evaluated queryset for a filter/search/order combination
def _get_queryset(model, queryset, filter_string, search_term, ordering_string, ordering_secondary=''):
    if filter_string:
        filter_query = _get_filtering_query(filter_string)
#        raise Exception(filter_query)
        queryset = queryset.filter(filter_query)
    else:
        queryset = queryset.all()

    if search_term:
        queryset = _get_search_results(search_term, queryset, model)

    if ordering_string:
        column, order = ordering_string.split(':')
        if column == re.sub('-','',ordering_secondary):
            ordering_secondary=''
        if order.lower() == DESCENDING:
            column = '-' + column
        if ordering_secondary:
            queryset = queryset.order_by(column, ordering_secondary)
        else:
            queryset = queryset.order_by(column)

    # insure only distinct records (e.g. from multiple search hits) are returned
    return queryset.distinct()

# returns the value of entries per page and the name of the applied sorting field.
# if the value is given explicitly as a GET parameter it will be the first selected,
# otherwise the cookie value will be used.
def _get_parameters_values(request, default_count, default_order):
    pagesize = request.GET.get('count', request.COOKIES.get('count', default_count))
    orderby = request.GET.get('orderby', request.COOKIES.get('orderby', default_order))
    return (pagesize, orderby)


# set cookies for parameters. this is usefull in case parameters are set
# manually from the GET values of the link
def _save_parameters_cookies(response, pagesize, orderby, request):
    html_parser = HTMLParser.HTMLParser()
    response.set_cookie(key='count', value=pagesize, path=request.path)
    response.set_cookie(key='orderby', value=html_parser.unescape(orderby), path=request.path)
    return response

# date range: normalize GUI's dd/mm/yyyy to date object
def _normalize_input_date(date_str,default):
    date_str=re.sub('/', '-', date_str)
    # accept dd/mm/yyyy to d/m/yy
    try:
        date_in = datetime.strptime(date_str, "%d-%m-%Y")
    except ValueError:
        # courtesy try with two digit year
        try:
            date_in = datetime.strptime(date_str, "%d-%m-%y")
        except ValueError:
            return default
    date_in = date_in.replace(tzinfo=default.tzinfo)
    return date_in

# convert and normalize any received date range filter, for example:
# "completed_on__gte!completed_on__lt:01/03/2015!02/03/2015_daterange" to
# "completed_on__gte!completed_on__lt:2015-03-01!2015-03-02"
def _modify_date_range_filter(filter_string):
    # was the date range radio button selected?
    if 0 >  filter_string.find('_daterange'):
        return filter_string,''
    # normalize GUI dates to database format
    filter_string = filter_string.replace('_daterange','').replace(':','!');
    filter_list = filter_string.split('!');
    if 4 != len(filter_list):
        return filter_string
    today = timezone.localtime(timezone.now())
    date_id = filter_list[1]
    date_from = _normalize_input_date(filter_list[2],today)
    date_to = _normalize_input_date(filter_list[3],today)
    # swap dates if manually set dates are out of order
    if  date_to < date_from:
        date_to,date_from = date_from,date_to
    # convert to strings, make 'date_to' inclusive by moving to begining of next day
    date_from_str = date_from.strftime("%Y-%m-%d")
    date_to_str = (date_to+timedelta(days=1)).strftime("%Y-%m-%d")
    filter_string=filter_list[0]+'!'+filter_list[1]+':'+date_from_str+'!'+date_to_str
    daterange_selected = re.sub('__.*','', date_id)
    return filter_string,daterange_selected

def _add_daterange_context(queryset_all, request, daterange_list):
    # calculate the exact begining of local today and yesterday
    today_begin = timezone.localtime(timezone.now())
    today_begin = date(today_begin.year,today_begin.month,today_begin.day)
    yesterday_begin = today_begin-timedelta(days=1)
    # add daterange persistent
    context_date = {}
    context_date['last_date_from'] = request.GET.get('last_date_from',timezone.localtime(timezone.now()).strftime("%d/%m/%Y"))
    context_date['last_date_to'  ] = request.GET.get('last_date_to'  ,context_date['last_date_from'])
    # calculate the date ranges, avoid second sort for 'created'
    # fetch the respective max range from the database
    context_date['daterange_filter']=''
    for key in daterange_list:
        queryset_key = queryset_all.order_by(key)
        try:
            context_date['dateMin_'+key]=timezone.localtime(getattr(queryset_key.first(),key)).strftime("%d/%m/%Y")
        except AttributeError:
            context_date['dateMin_'+key]=timezone.localtime(timezone.now())
        try:
            context_date['dateMax_'+key]=timezone.localtime(getattr(queryset_key.last(),key)).strftime("%d/%m/%Y")
        except AttributeError:
            context_date['dateMax_'+key]=timezone.localtime(timezone.now())
    return context_date,today_begin,yesterday_begin


##
# build dashboard for a single build, coming in as argument
# Each build may contain multiple targets and each target
# may generate multiple image files. display them all.
#
def builddashboard( request, build_id ):
    template = "builddashboard.html"
    if Build.objects.filter( pk=build_id ).count( ) == 0 :
        return redirect( builds )
    build = Build.objects.get( pk = build_id );
    layerVersionId = Layer_Version.objects.filter( build = build_id );
    recipeCount = Recipe.objects.filter( layer_version__id__in = layerVersionId ).count( );
    tgts = Target.objects.filter( build_id = build_id ).order_by( 'target' );

    ##
    # set up custom target list with computed package and image data
    #

    targets = [ ]
    ntargets = 0
    hasImages = False
    targetHasNoImages = False
    for t in tgts:
        elem = { }
        elem[ 'target' ] = t
        if ( t.is_image ):
            hasImages = True
        npkg = 0
        pkgsz = 0
        package = None
        for package in Package.objects.filter(id__in = [x.package_id for x in t.target_installed_package_set.all()]):
            pkgsz = pkgsz + package.size
            if ( package.installed_name ):
                npkg = npkg + 1
        elem[ 'npkg' ] = npkg
        elem[ 'pkgsz' ] = pkgsz
        ti = Target_Image_File.objects.filter( target_id = t.id )
        imageFiles = [ ]
        for i in ti:
            ndx = i.file_name.rfind( '/' )
            if ( ndx < 0 ):
                ndx = 0;
            f = i.file_name[ ndx + 1: ]
            imageFiles.append({ 'id': i.id, 'path': f, 'size' : i.file_size })
        if ( t.is_image and
             (( len( imageFiles ) <= 0 ) or ( len( t.license_manifest_path ) <= 0 ))):
            targetHasNoImages = True
        elem[ 'imageFiles' ] = imageFiles
        elem[ 'targetHasNoImages' ] = targetHasNoImages
        targets.append( elem )

    ##
    # how many packages in this build - ignore anonymous ones
    #

    packageCount = 0
    packages = Package.objects.filter( build_id = build_id )
    for p in packages:
        if ( p.installed_name ):
            packageCount = packageCount + 1

    logmessages = list(LogMessage.objects.filter( build = build_id ))

    context = {
            'build'           : build,
            'hasImages'       : hasImages,
            'ntargets'        : ntargets,
            'targets'         : targets,
            'recipecount'     : recipeCount,
            'packagecount'    : packageCount,
            'logmessages'     : logmessages,
    }
    return render( request, template, context )



def generateCoveredList2( revlist = [] ):
    covered_list =  [ x for x in revlist if x.outcome == Task.OUTCOME_COVERED ]
    while len(covered_list):
        revlist =  [ x for x in revlist if x.outcome != Task.OUTCOME_COVERED ]
        if len(revlist) > 0:
            return revlist

        newlist = _find_task_revdep_list(covered_list)

        revlist = list(set(revlist + newlist))
        covered_list =  [ x for x in revlist if x.outcome == Task.OUTCOME_COVERED ]
    return revlist

def task( request, build_id, task_id ):
    template = "task.html"
    tasks = Task.objects.filter( pk=task_id )
    if tasks.count( ) == 0:
        return redirect( builds )
    task = tasks[ 0 ];
    dependencies = sorted(
        _find_task_dep( task ),
        key=lambda t:'%s_%s %s'%(t.recipe.name, t.recipe.version, t.task_name))
    reverse_dependencies = sorted(
        _find_task_revdep( task ),
        key=lambda t:'%s_%s %s'%( t.recipe.name, t.recipe.version, t.task_name ))
    coveredBy = '';
    if ( task.outcome == Task.OUTCOME_COVERED ):
#        _list = generateCoveredList( task )
        coveredBy = sorted(generateCoveredList2( _find_task_revdep( task ) ), key = lambda x: x.recipe.name)
    log_head = ''
    log_body = ''
    if task.outcome == task.OUTCOME_FAILED:
        pass

    uri_list= [ ]
    variables = Variable.objects.filter(build=build_id)
    v=variables.filter(variable_name='SSTATE_DIR')
    if v.count > 0:
        uri_list.append(v[0].variable_value)
    v=variables.filter(variable_name='SSTATE_MIRRORS')
    if (v.count > 0):
        for mirror in v[0].variable_value.split('\\n'):
            s=re.sub('.* ','',mirror.strip(' \t\n\r'))
            if len(s): uri_list.append(s)

    context = {
            'build'           : Build.objects.filter( pk = build_id )[ 0 ],
            'object'          : task,
            'task'            : task,
            'covered_by'      : coveredBy,
            'deps'            : dependencies,
            'rdeps'           : reverse_dependencies,
            'log_head'        : log_head,
            'log_body'        : log_body,
            'showing_matches' : False,
            'uri_list'        : uri_list,
    }
    if request.GET.get( 'show_matches', "" ):
        context[ 'showing_matches' ] = True
        context[ 'matching_tasks' ] = Task.objects.filter(
            sstate_checksum=task.sstate_checksum ).filter(
            build__completed_on__lt=task.build.completed_on).exclude(
            order__isnull=True).exclude(outcome=Task.OUTCOME_NA).order_by('-build__completed_on')

    return render( request, template, context )

def recipe(request, build_id, recipe_id, active_tab="1"):
    template = "recipe.html"
    if Recipe.objects.filter(pk=recipe_id).count() == 0 :
        return redirect(builds)

    object = Recipe.objects.get(pk=recipe_id)
    layer_version = Layer_Version.objects.get(pk=object.layer_version_id)
    layer  = Layer.objects.get(pk=layer_version.layer_id)
    tasks  = Task.objects.filter(recipe_id = recipe_id, build_id = build_id).exclude(order__isnull=True).exclude(task_name__endswith='_setscene').exclude(outcome=Task.OUTCOME_NA)
    package_count = Package.objects.filter(recipe_id = recipe_id).filter(build_id = build_id).filter(size__gte=0).count()

    if active_tab != '1' and active_tab != '3' and active_tab != '4' :
        active_tab = '1'
    tab_states = {'1': '', '3': '', '4': ''}
    tab_states[active_tab] = 'active'

    context = {
            'build'   : Build.objects.get(pk=build_id),
            'object'  : object,
            'layer_version' : layer_version,
            'layer'   : layer,
            'tasks'   : tasks,
            'package_count' : package_count,
            'tab_states' : tab_states,
    }
    return render(request, template, context)

def recipe_packages(request, build_id, recipe_id):
    template = "recipe_packages.html"
    if Recipe.objects.filter(pk=recipe_id).count() == 0 :
        return redirect(builds)

    (pagesize, orderby) = _get_parameters_values(request, 10, 'name:+')
    mandatory_parameters = { 'count': pagesize,  'page' : 1, 'orderby': orderby }
    retval = _verify_parameters( request.GET, mandatory_parameters )
    if retval:
        return _redirect_parameters( 'recipe_packages', request.GET, mandatory_parameters, build_id = build_id, recipe_id = recipe_id)
    (filter_string, search_term, ordering_string) = _search_tuple(request, Package)

    recipe = Recipe.objects.get(pk=recipe_id)
    queryset = Package.objects.filter(recipe_id = recipe_id).filter(build_id = build_id).filter(size__gte=0)
    package_count = queryset.count()
    queryset = _get_queryset(Package, queryset, filter_string, search_term, ordering_string, 'name')

    packages = _build_page_range(Paginator(queryset, pagesize),request.GET.get('page', 1))

    context = {
            'build'   : Build.objects.get(pk=build_id),
            'recipe'  : recipe,
            'objects'  : packages,
            'object_count' : package_count,
            'tablecols':[
                {
                    'name':'Package',
                    'orderfield': _get_toggle_order(request,"name"),
                    'ordericon': _get_toggle_order_icon(request,"name"),
                    'orderkey': "name",
                },
                {
                    'name':'Version',
                },
                {
                    'name':'Size',
                    'orderfield': _get_toggle_order(request,"size", True),
                    'ordericon': _get_toggle_order_icon(request,"size"),
                    'orderkey': 'size',
                    'dclass': 'sizecol span2',
                },
           ]
       }
    response = render(request, template, context)
    _save_parameters_cookies(response, pagesize, orderby, request)
    return response

def target_common( request, build_id, target_id, variant ):
    template = "target.html"
    (pagesize, orderby) = _get_parameters_values(request, 25, 'name:+')
    mandatory_parameters = { 'count': pagesize,  'page' : 1, 'orderby': orderby }
    retval = _verify_parameters( request.GET, mandatory_parameters )
    if retval:
        return _redirect_parameters(
                    variant, request.GET, mandatory_parameters,
                    build_id = build_id, target_id = target_id )
    ( filter_string, search_term, ordering_string ) = _search_tuple( request, Package )

    # FUTURE:  get rid of nested sub-queries replacing with ManyToMany field
    queryset = Package.objects.filter(
                    size__gte = 0,
                    id__in = Target_Installed_Package.objects.filter(
                        target_id=target_id ).values( 'package_id' ))
    packages_sum =  queryset.aggregate( Sum( 'installed_size' ))
    queryset = _get_queryset(
            Package, queryset, filter_string, search_term, ordering_string, 'name' )
    queryset = queryset.select_related("recipe", "recipe__layer_version", "recipe__layer_version__layer")
    packages = _build_page_range( Paginator(queryset, pagesize), request.GET.get( 'page', 1 ))



    build = Build.objects.get( pk = build_id )

    # bring in package dependencies
    for p in packages.object_list:
        p.runtime_dependencies = p.package_dependencies_source.filter(
            target_id = target_id, dep_type=Package_Dependency.TYPE_TRDEPENDS ).select_related("depends_on")
        p.reverse_runtime_dependencies = p.package_dependencies_target.filter(
            target_id = target_id, dep_type=Package_Dependency.TYPE_TRDEPENDS ).select_related("package")
    tc_package = {
        'name'       : 'Package',
        'qhelp'      : 'Packaged output resulting from building a recipe included in this image',
        'orderfield' : _get_toggle_order( request, "name" ),
        'ordericon'  : _get_toggle_order_icon( request, "name" ),
        }
    tc_packageVersion = {
        'name'       : 'Package version',
        'qhelp'      : 'The package version and revision',
        }
    tc_size = {
        'name'       : 'Size',
        'qhelp'      : 'The size of the package',
        'orderfield' : _get_toggle_order( request, "size", True ),
        'ordericon'  : _get_toggle_order_icon( request, "size" ),
        'orderkey'   : 'size',
        'clclass'    : 'size',
        'dclass'     : 'span2',
        }
    if ( variant == 'target' ):
        tc_size[ "hidden" ] = 0
    else:
        tc_size[ "hidden" ] = 1
    tc_sizePercentage = {
        'name'       : 'Size over total (%)',
        'qhelp'      : 'Proportion of the overall size represented by this package',
        'clclass'    : 'size_over_total',
        'hidden'     : 1,
        }
    tc_license = {
        'name'       : 'License',
        'qhelp'      : 'The license under which the package is distributed. Separate license names u\
sing | (pipe) means there is a choice between licenses. Separate license names using & (ampersand) m\
eans multiple licenses exist that cover different parts of the source',
        'orderfield' : _get_toggle_order( request, "license" ),
        'ordericon'  : _get_toggle_order_icon( request, "license" ),
        'orderkey'   : 'license',
        'clclass'    : 'license',
        }
    if ( variant == 'target' ):
        tc_license[ "hidden" ] = 1
    else:
        tc_license[ "hidden" ] = 0
    tc_dependencies = {
        'name'       : 'Dependencies',
        'qhelp'      : "Package runtime dependencies (other packages)",
        'clclass'    : 'depends',
        }
    if ( variant == 'target' ):
        tc_dependencies[ "hidden" ] = 0
    else:
        tc_dependencies[ "hidden" ] = 1
    tc_rdependencies = {
        'name'       : 'Reverse dependencies',
        'qhelp'      : 'Package run-time reverse dependencies (i.e. which other packages depend on this package',
        'clclass'    : 'brought_in_by',
        }
    if ( variant == 'target' ):
        tc_rdependencies[ "hidden" ] = 0
    else:
        tc_rdependencies[ "hidden" ] = 1
    tc_recipe = {
        'name'       : 'Recipe',
        'qhelp'      : 'The name of the recipe building the package',
        'orderfield' : _get_toggle_order( request, "recipe__name" ),
        'ordericon'  : _get_toggle_order_icon( request, "recipe__name" ),
        'orderkey'   : "recipe__name",
        'clclass'    : 'recipe_name',
        'hidden'     : 0,
        }
    tc_recipeVersion = {
        'name'       : 'Recipe version',
        'qhelp'      : 'Version and revision of the recipe building the package',
        'clclass'    : 'recipe_version',
        'hidden'     : 1,
        }
    tc_layer = {
        'name'       : 'Layer',
        'qhelp'      : 'The name of the layer providing the recipe that builds the package',
        'orderfield' : _get_toggle_order( request, "recipe__layer_version__layer__name" ),
        'ordericon'  : _get_toggle_order_icon( request, "recipe__layer_version__layer__name" ),
        'orderkey'   : "recipe__layer_version__layer__name",
        'clclass'    : 'layer_name',
        'hidden'     : 1,
        }
    tc_layerBranch = {
        'name'       : 'Layer branch',
        'qhelp'      : 'The Git branch of the layer providing the recipe that builds the package',
        'orderfield' : _get_toggle_order( request, "recipe__layer_version__branch" ),
        'ordericon'  : _get_toggle_order_icon( request, "recipe__layer_version__branch" ),
        'orderkey'   : "recipe__layer_version__branch",
        'clclass'    : 'layer_branch',
        'hidden'     : 1,
        }
    tc_layerCommit = {
        'name'       : 'Layer commit',
        'qhelp'      : 'The Git commit of the layer providing the recipe that builds the package',
        'clclass'    : 'layer_commit',
        'hidden'     : 1,
        }

    context = {
        'objectname': variant,
        'build'                : build,
        'target'               : Target.objects.filter( pk = target_id )[ 0 ],
        'objects'              : packages,
        'packages_sum'         : packages_sum[ 'installed_size__sum' ],
        'object_search_display': "packages included",
        'default_orderby'      : orderby,
        'tablecols'            : [
                    tc_package,
                    tc_packageVersion,
                    tc_license,
                    tc_size,
                    tc_sizePercentage,
                    tc_dependencies,
                    tc_rdependencies,
                    tc_recipe,
                    tc_recipeVersion,
                    tc_layer,
                    tc_layerBranch,
                    tc_layerCommit,
                ]
        }

    if not toastermain.settings.MANAGED or build.project is None:

        tc_layerDir = {
            'name':'Layer directory',
            'qhelp':'Location in disk of the layer providing the recipe that builds the package',
            'orderfield' : _get_toggle_order( request, "recipe__layer_version__layer__local_path" ),
            'ordericon'  : _get_toggle_order_icon( request, "recipe__layer_version__layer__local_path" ),
            'orderkey'   : "recipe__layer_version__layer__local_path",
            'clclass'    : 'layer_directory',
            'hidden'     : 1,
        }
        context['tablecols'].append(tc_layerDir)

    response = render(request, template, context)
    _save_parameters_cookies(response, pagesize, orderby, request)
    return response

def target( request, build_id, target_id ):
    return( target_common( request, build_id, target_id, "target" ))

def targetpkg( request, build_id, target_id ):
    return( target_common( request, build_id, target_id, "targetpkg" ))

from django.core.serializers.json import DjangoJSONEncoder
from django.http import HttpResponse
def dirinfo_ajax(request, build_id, target_id):
    top = request.GET.get('start', '/')
    return HttpResponse(_get_dir_entries(build_id, target_id, top))

from django.utils.functional import Promise
from django.utils.encoding import force_text
class LazyEncoder(json.JSONEncoder):
    def default(self, obj):
        if isinstance(obj, Promise):
            return force_text(obj)
        return super(LazyEncoder, self).default(obj)

from toastergui.templatetags.projecttags import filtered_filesizeformat
import os
def _get_dir_entries(build_id, target_id, start):
    node_str = {
        Target_File.ITYPE_REGULAR   : '-',
        Target_File.ITYPE_DIRECTORY : 'd',
        Target_File.ITYPE_SYMLINK   : 'l',
        Target_File.ITYPE_SOCKET    : 's',
        Target_File.ITYPE_FIFO      : 'p',
        Target_File.ITYPE_CHARACTER : 'c',
        Target_File.ITYPE_BLOCK     : 'b',
    }
    response = []
    objects  = Target_File.objects.filter(target__exact=target_id, directory__path=start)
    target_packages = Target_Installed_Package.objects.filter(target__exact=target_id).values_list('package_id', flat=True)
    for o in objects:
        # exclude root inode '/'
        if o.path == '/':
            continue
        try:
            entry = {}
            entry['parent'] = start
            entry['name'] = os.path.basename(o.path)
            entry['fullpath'] = o.path

            # set defaults, not all dentries have packages
            entry['installed_package'] = None
            entry['package_id'] = None
            entry['package'] = None
            entry['link_to'] = None
            if o.inodetype == Target_File.ITYPE_DIRECTORY:
                entry['isdir'] = 1
                # is there content in directory
                entry['childcount'] = Target_File.objects.filter(target__exact=target_id, directory__path=o.path).all().count()
            else:
                entry['isdir'] = 0

                # resolve the file to get the package from the resolved file
                resolved_id = o.sym_target_id
                resolved_path = o.path
                if target_packages.count():
                    while resolved_id != "" and resolved_id != None:
                        tf = Target_File.objects.get(pk=resolved_id)
                        resolved_path = tf.path
                        resolved_id = tf.sym_target_id

                    thisfile=Package_File.objects.all().filter(path__exact=resolved_path, package_id__in=target_packages)
                    if thisfile.count():
                        p = Package.objects.get(pk=thisfile[0].package_id)
                        entry['installed_package'] = p.installed_name
                        entry['package_id'] = str(p.id)
                        entry['package'] = p.name
                # don't use resolved path from above, show immediate link-to
                if o.sym_target_id != "" and o.sym_target_id != None:
                    entry['link_to'] = Target_File.objects.get(pk=o.sym_target_id).path
            entry['size'] = filtered_filesizeformat(o.size)
            if entry['link_to'] != None:
                entry['permission'] = node_str[o.inodetype] + o.permission
            else:
                entry['permission'] = node_str[o.inodetype] + o.permission
            entry['owner'] = o.owner
            entry['group'] = o.group
            response.append(entry)

        except Exception as e:
            print "Exception ", e
            import traceback
            traceback.print_exc(e)
            pass

    # sort by directories first, then by name
    rsorted = sorted(response, key=lambda entry :  entry['name'])
    rsorted = sorted(rsorted, key=lambda entry :  entry['isdir'], reverse=True)
    return json.dumps(rsorted, cls=LazyEncoder).replace('</', '<\\/')

def dirinfo(request, build_id, target_id, file_path=None):
    template = "dirinfo.html"
    objects = _get_dir_entries(build_id, target_id, '/')
    packages_sum = Package.objects.filter(id__in=Target_Installed_Package.objects.filter(target_id=target_id).values('package_id')).aggregate(Sum('installed_size'))
    dir_list = None
    if file_path != None:
        """
        Link from the included package detail file list page and is
        requesting opening the dir info to a specific file path.
        Provide the list of directories to expand and the full path to
        highlight in the page.
        """
        # Aassume target's path separator matches host's, that is, os.sep
        sep = os.sep
        dir_list = []
        head = file_path
        while head != sep:
            (head,tail) = os.path.split(head)
            if head != sep:
                dir_list.insert(0, head)

    context = { 'build': Build.objects.get(pk=build_id),
                'target': Target.objects.get(pk=target_id),
                'packages_sum': packages_sum['installed_size__sum'],
                'objects': objects,
                'dir_list': dir_list,
                'file_path': file_path,
              }
    return render(request, template, context)

def _find_task_dep(task):
    return map(lambda x: x.depends_on, Task_Dependency.objects.filter(task=task).filter(depends_on__order__gt = 0).exclude(depends_on__outcome = Task.OUTCOME_NA).select_related("depends_on"))


def _find_task_revdep(task):
    tp = []
    tp = map(lambda t: t.task, Task_Dependency.objects.filter(depends_on=task).filter(task__order__gt=0).exclude(task__outcome = Task.OUTCOME_NA).select_related("task", "task__recipe", "task__build"))
    return tp

def _find_task_revdep_list(tasklist):
    tp = []
    tp = map(lambda t: t.task, Task_Dependency.objects.filter(depends_on__in=tasklist).filter(task__order__gt=0).exclude(task__outcome = Task.OUTCOME_NA).select_related("task", "task__recipe", "task__build"))
    return tp

def _find_task_provider(task):
    task_revdeps = _find_task_revdep(task)
    for tr in task_revdeps:
        if tr.outcome != Task.OUTCOME_COVERED:
            return tr
    for tr in task_revdeps:
        trc = _find_task_provider(tr)
        if trc is not None:
            return trc
    return None

def tasks_common(request, build_id, variant, task_anchor):
# This class is shared between these pages
#
# Column    tasks  buildtime  diskio  cpuusage
# --------- ------ ---------- ------- ---------
# Cache      def
# CPU                                   min -
# Disk                         min -
# Executed   def     def       def      def
# Log
# Order      def +
# Outcome    def     def       def      def
# Recipe     min     min       min      min
# Version
# Task       min     min       min      min
# Time               min -
#
# 'min':on always, 'def':on by default, else hidden
# '+' default column sort up, '-' default column sort down

    anchor = request.GET.get('anchor', '')
    if not anchor:
        anchor=task_anchor

    # default ordering depends on variant
    if   'buildtime' == variant:
        title_variant='Time'
        object_search_display="time data"
        filter_search_display="tasks"
        (pagesize, orderby) = _get_parameters_values(request, 25, 'elapsed_time:-')
    elif 'diskio'    == variant:
        title_variant='Disk I/O'
        object_search_display="disk I/O data"
        filter_search_display="tasks"
        (pagesize, orderby) = _get_parameters_values(request, 25, 'disk_io:-')
    elif 'cpuusage'  == variant:
        title_variant='CPU usage'
        object_search_display="CPU usage data"
        filter_search_display="tasks"
        (pagesize, orderby) = _get_parameters_values(request, 25, 'cpu_usage:-')
    else :
        title_variant='Tasks'
        object_search_display="tasks"
        filter_search_display="tasks"
        (pagesize, orderby) = _get_parameters_values(request, 25, 'order:+')


    mandatory_parameters = { 'count': pagesize,  'page' : 1, 'orderby': orderby }

    template = 'tasks.html'
    retval = _verify_parameters( request.GET, mandatory_parameters )
    if retval:
        if task_anchor:
            mandatory_parameters['anchor']=task_anchor
        return _redirect_parameters( variant, request.GET, mandatory_parameters, build_id = build_id)
    (filter_string, search_term, ordering_string) = _search_tuple(request, Task)
    queryset_all = Task.objects.filter(build=build_id).exclude(order__isnull=True).exclude(outcome=Task.OUTCOME_NA)
    queryset_all = queryset_all.select_related("recipe", "build")

    queryset_with_search = _get_queryset(Task, queryset_all, None , search_term, ordering_string, 'order')

    if ordering_string.startswith('outcome'):
        queryset = _get_queryset(Task, queryset_all, filter_string, search_term, 'order:+', 'order')
        queryset = sorted(queryset, key=lambda ur: (ur.outcome_text), reverse=ordering_string.endswith('-'))
    elif ordering_string.startswith('sstate_result'):
        queryset = _get_queryset(Task, queryset_all, filter_string, search_term, 'order:+', 'order')
        queryset = sorted(queryset, key=lambda ur: (ur.sstate_text), reverse=ordering_string.endswith('-'))
    else:
        queryset = _get_queryset(Task, queryset_all, filter_string, search_term, ordering_string, 'order')


    # compute the anchor's page
    if anchor:
        request.GET = request.GET.copy()
        del request.GET['anchor']
        i=0
        a=int(anchor)
        count_per_page=int(pagesize)
        for task in queryset.iterator():
            if a == task.order:
                new_page= (i / count_per_page ) + 1
                request.GET.__setitem__('page', new_page)
                mandatory_parameters['page']=new_page
                return _redirect_parameters( variant, request.GET, mandatory_parameters, build_id = build_id)
            i += 1

    tasks = _build_page_range(Paginator(queryset, pagesize),request.GET.get('page', 1))

    # define (and modify by variants) the 'tablecols' members
    tc_order={
        'name':'Order',
        'qhelp':'The running sequence of each task in the build',
        'clclass': 'order', 'hidden' : 1,
        'orderkey' : 'order',
        'orderfield':_get_toggle_order(request, "order"),
        'ordericon':_get_toggle_order_icon(request, "order")}
    if 'tasks' == variant: tc_order['hidden']='0'; del tc_order['clclass']
    tc_recipe={
        'name':'Recipe',
        'qhelp':'The name of the recipe to which each task applies',
        'orderkey' : 'recipe__name',
        'orderfield': _get_toggle_order(request, "recipe__name"),
        'ordericon':_get_toggle_order_icon(request, "recipe__name"),
    }
    tc_recipe_version={
        'name':'Recipe version',
        'qhelp':'The version of the recipe to which each task applies',
        'clclass': 'recipe_version', 'hidden' : 1,
    }
    tc_task={
        'name':'Task',
        'qhelp':'The name of the task',
        'orderfield': _get_toggle_order(request, "task_name"),
        'ordericon':_get_toggle_order_icon(request, "task_name"),
        'orderkey' : 'task_name',
    }
    tc_executed={
        'name':'Executed',
        'qhelp':"This value tells you if a task had to run (executed) in order to generate the task output, or if the output was provided by another task and therefore the task didn't need to run (not executed)",
        'clclass': 'executed', 'hidden' : 0,
        'orderfield': _get_toggle_order(request, "task_executed"),
        'ordericon':_get_toggle_order_icon(request, "task_executed"),
        'orderkey' : 'task_executed',
        'filter' : {
                   'class' : 'executed',
                   'label': 'Show:',
                   'options' : [
                               ('Executed Tasks', 'task_executed:1', queryset_with_search.filter(task_executed=1).count()),
                               ('Not Executed Tasks', 'task_executed:0', queryset_with_search.filter(task_executed=0).count()),
                               ]
                   }

    }
    tc_outcome={
        'name':'Outcome',
        'qhelp':"This column tells you if 'executed' tasks succeeded or failed. The column also tells you why 'not executed' tasks did not need to run",
        'clclass': 'outcome', 'hidden' : 0,
        'orderfield': _get_toggle_order(request, "outcome"),
        'ordericon':_get_toggle_order_icon(request, "outcome"),
        'orderkey' : 'outcome',
        'filter' : {
                   'class' : 'outcome',
                   'label': 'Show:',
                   'options' : [
                               ('Succeeded Tasks', 'outcome:%d'%Task.OUTCOME_SUCCESS, queryset_with_search.filter(outcome=Task.OUTCOME_SUCCESS).count(), "'Succeeded' tasks are those that ran and completed during the build" ),
                               ('Failed Tasks', 'outcome:%d'%Task.OUTCOME_FAILED, queryset_with_search.filter(outcome=Task.OUTCOME_FAILED).count(), "'Failed' tasks are those that ran but did not complete during the build"),
                               ('Cached Tasks', 'outcome:%d'%Task.OUTCOME_CACHED, queryset_with_search.filter(outcome=Task.OUTCOME_CACHED).count(), 'Cached tasks restore output from the <code>sstate-cache</code> directory or mirrors'),
                               ('Prebuilt Tasks', 'outcome:%d'%Task.OUTCOME_PREBUILT, queryset_with_search.filter(outcome=Task.OUTCOME_PREBUILT).count(),'Prebuilt tasks didn\'t need to run because their output was reused from a previous build'),
                               ('Covered Tasks', 'outcome:%d'%Task.OUTCOME_COVERED, queryset_with_search.filter(outcome=Task.OUTCOME_COVERED).count(), 'Covered tasks didn\'t need to run because their output is provided by another task in this build'),
                               ('Empty Tasks', 'outcome:%d'%Task.OUTCOME_EMPTY, queryset_with_search.filter(outcome=Task.OUTCOME_EMPTY).count(), 'Empty tasks have no executable content'),
                               ]
                   }

    }
    tc_log={
        'name':'Log',
        'qhelp':'Path to the task log file',
        'orderfield': _get_toggle_order(request, "logfile"),
        'ordericon':_get_toggle_order_icon(request, "logfile"),
        'orderkey' : 'logfile',
        'clclass': 'task_log', 'hidden' : 1,
    }
    tc_cache={
        'name':'Cache attempt',
        'qhelp':'This column tells you if a task tried to restore output from the <code>sstate-cache</code> directory or mirrors, and reports the result: Succeeded, Failed or File not in cache',
        'clclass': 'cache_attempt', 'hidden' : 0,
        'orderfield': _get_toggle_order(request, "sstate_result"),
        'ordericon':_get_toggle_order_icon(request, "sstate_result"),
        'orderkey' : 'sstate_result',
        'filter' : {
                   'class' : 'cache_attempt',
                   'label': 'Show:',
                   'options' : [
                               ('Tasks with cache attempts', 'sstate_result__gt:%d'%Task.SSTATE_NA, queryset_with_search.filter(sstate_result__gt=Task.SSTATE_NA).count(), 'Show all tasks that tried to restore ouput from the <code>sstate-cache</code> directory or mirrors'),
                               ("Tasks with 'File not in cache' attempts", 'sstate_result:%d'%Task.SSTATE_MISS,  queryset_with_search.filter(sstate_result=Task.SSTATE_MISS).count(), 'Show tasks that tried to restore output, but did not find it in the <code>sstate-cache</code> directory or mirrors'),
                               ("Tasks with 'Failed' cache attempts", 'sstate_result:%d'%Task.SSTATE_FAILED,  queryset_with_search.filter(sstate_result=Task.SSTATE_FAILED).count(), 'Show tasks that found the required output in the <code>sstate-cache</code> directory or mirrors, but could not restore it'),
                               ("Tasks with 'Succeeded' cache attempts", 'sstate_result:%d'%Task.SSTATE_RESTORED,  queryset_with_search.filter(sstate_result=Task.SSTATE_RESTORED).count(), 'Show tasks that successfully restored the required output from the <code>sstate-cache</code> directory or mirrors'),
                               ]
                   }

    }
    #if   'tasks' == variant: tc_cache['hidden']='0';
    tc_time={
        'name':'Time (secs)',
        'qhelp':'How long it took the task to finish in seconds',
        'orderfield': _get_toggle_order(request, "elapsed_time", True),
        'ordericon':_get_toggle_order_icon(request, "elapsed_time"),
        'orderkey' : 'elapsed_time',
        'clclass': 'time_taken', 'hidden' : 1,
    }
    if   'buildtime' == variant: tc_time['hidden']='0'; del tc_time['clclass']; tc_cache['hidden']='1';
    tc_cpu={
        'name':'CPU usage',
        'qhelp':'The percentage of task CPU utilization',
        'orderfield': _get_toggle_order(request, "cpu_usage", True),
        'ordericon':_get_toggle_order_icon(request, "cpu_usage"),
        'orderkey' : 'cpu_usage',
        'clclass': 'cpu_used', 'hidden' : 1,
    }
    if   'cpuusage' == variant: tc_cpu['hidden']='0'; del tc_cpu['clclass']; tc_cache['hidden']='1';
    tc_diskio={
        'name':'Disk I/O (ms)',
        'qhelp':'Number of miliseconds the task spent doing disk input and output',
        'orderfield': _get_toggle_order(request, "disk_io", True),
        'ordericon':_get_toggle_order_icon(request, "disk_io"),
        'orderkey' : 'disk_io',
        'clclass': 'disk_io', 'hidden' : 1,
    }
    if   'diskio' == variant: tc_diskio['hidden']='0'; del tc_diskio['clclass']; tc_cache['hidden']='1';

    build = Build.objects.get(pk=build_id)

    context = { 'objectname': variant,
                'object_search_display': object_search_display,
                'filter_search_display': filter_search_display,
                'title': title_variant,
                'build': build,
                'objects': tasks,
                'default_orderby' : orderby,
                'search_term': search_term,
                'total_count': queryset_with_search.count(),
                'tablecols':[
                    tc_order,
                    tc_recipe,
                    tc_recipe_version,
                    tc_task,
                    tc_executed,
                    tc_outcome,
                    tc_cache,
                    tc_time,
                    tc_cpu,
                    tc_diskio,
                ]}


    if not toastermain.settings.MANAGED or build.project is None:
        context['tablecols'].append(tc_log)

    response = render(request, template, context)
    _save_parameters_cookies(response, pagesize, orderby, request)
    return response

def tasks(request, build_id):
    return tasks_common(request, build_id, 'tasks', '')

def tasks_task(request, build_id, task_id):
    return tasks_common(request, build_id, 'tasks', task_id)

def buildtime(request, build_id):
    return tasks_common(request, build_id, 'buildtime', '')

def diskio(request, build_id):
    return tasks_common(request, build_id, 'diskio', '')

def cpuusage(request, build_id):
    return tasks_common(request, build_id, 'cpuusage', '')


def recipes(request, build_id):
    template = 'recipes.html'
    (pagesize, orderby) = _get_parameters_values(request, 100, 'name:+')
    mandatory_parameters = { 'count': pagesize,  'page' : 1, 'orderby' : orderby }
    retval = _verify_parameters( request.GET, mandatory_parameters )
    if retval:
        return _redirect_parameters( 'recipes', request.GET, mandatory_parameters, build_id = build_id)
    (filter_string, search_term, ordering_string) = _search_tuple(request, Recipe)
    queryset = Recipe.objects.filter(layer_version__id__in=Layer_Version.objects.filter(build=build_id)).select_related("layer_version", "layer_version__layer")
    queryset = _get_queryset(Recipe, queryset, filter_string, search_term, ordering_string, 'name')

    recipes = _build_page_range(Paginator(queryset, pagesize),request.GET.get('page', 1))

    # prefetch the forward and reverse recipe dependencies
    deps = { }; revs = { }
    queryset_dependency=Recipe_Dependency.objects.filter(recipe__layer_version__build_id = build_id).select_related("depends_on", "recipe")
    for recipe in recipes:
        deplist = [ ]
        for recipe_dep in [x for x in queryset_dependency if x.recipe_id == recipe.id]:
            deplist.append(recipe_dep)
        deps[recipe.id] = deplist
        revlist = [ ]
        for recipe_dep in [x for x in queryset_dependency if x.depends_on_id == recipe.id]:
            revlist.append(recipe_dep)
        revs[recipe.id] = revlist

    build = Build.objects.get(pk=build_id)

    context = {
        'objectname': 'recipes',
        'build': build,
        'objects': recipes,
        'default_orderby' : 'name:+',
        'recipe_deps' : deps,
        'recipe_revs' : revs,
        'tablecols':[
            {
                'name':'Recipe',
                'qhelp':'Information about a single piece of software, including where to download the source, configuration options, how to compile the source files and how to package the compiled output',
                'orderfield': _get_toggle_order(request, "name"),
                'ordericon':_get_toggle_order_icon(request, "name"),
            },
            {
                'name':'Recipe version',
                'qhelp':'The recipe version and revision',
            },
            {
                'name':'Dependencies',
                'qhelp':'Recipe build-time dependencies (i.e. other recipes)',
                'clclass': 'depends_on', 'hidden': 1,
            },
            {
                'name':'Reverse dependencies',
                'qhelp':'Recipe build-time reverse dependencies (i.e. the recipes that depend on this recipe)',
                'clclass': 'depends_by', 'hidden': 1,
            },
            {
                'name':'Recipe file',
                'qhelp':'Path to the recipe .bb file',
                'orderfield': _get_toggle_order(request, "file_path"),
                'ordericon':_get_toggle_order_icon(request, "file_path"),
                'orderkey' : 'file_path',
                'clclass': 'recipe_file', 'hidden': 0,
            },
            {
                'name':'Section',
                'qhelp':'The section in which recipes should be categorized',
                'orderfield': _get_toggle_order(request, "section"),
                'ordericon':_get_toggle_order_icon(request, "section"),
                'orderkey' : 'section',
                'clclass': 'recipe_section', 'hidden': 0,
            },
            {
                'name':'License',
                'qhelp':'The list of source licenses for the recipe. Multiple license names separated by the pipe character indicates a choice between licenses. Multiple license names separated by the ampersand character indicates multiple licenses exist that cover different parts of the source',
                'orderfield': _get_toggle_order(request, "license"),
                'ordericon':_get_toggle_order_icon(request, "license"),
                'orderkey' : 'license',
                'clclass': 'recipe_license', 'hidden': 0,
            },
            {
                'name':'Layer',
                'qhelp':'The name of the layer providing the recipe',
                'orderfield': _get_toggle_order(request, "layer_version__layer__name"),
                'ordericon':_get_toggle_order_icon(request, "layer_version__layer__name"),
                'orderkey' : 'layer_version__layer__name',
                'clclass': 'layer_version__layer__name', 'hidden': 0,
            },
            {
                'name':'Layer branch',
                'qhelp':'The Git branch of the layer providing the recipe',
                'orderfield': _get_toggle_order(request, "layer_version__branch"),
                'ordericon':_get_toggle_order_icon(request, "layer_version__branch"),
                'orderkey' : 'layer_version__branch',
                'clclass': 'layer_version__branch', 'hidden': 1,
            },
            {
                'name':'Layer commit',
                'qhelp':'The Git commit of the layer providing the recipe',
                'clclass': 'layer_version__layer__commit', 'hidden': 1,
            },
            ]
        }

    if not toastermain.settings.MANAGED or build.project is None:
        context['tablecols'].append(
            {
                'name':'Layer directory',
                'qhelp':'Path to the layer prodiving the recipe',
                'orderfield': _get_toggle_order(request, "layer_version__layer__local_path"),
                'ordericon':_get_toggle_order_icon(request, "layer_version__layer__local_path"),
                'orderkey' : 'layer_version__layer__local_path',
                'clclass': 'layer_version__layer__local_path', 'hidden': 1,
            })


    response = render(request, template, context)
    _save_parameters_cookies(response, pagesize, orderby, request)
    return response

def configuration(request, build_id):
    template = 'configuration.html'

    variables = Variable.objects.filter(build=build_id)

    def _get_variable_or_empty(variable_name):
        from django.core.exceptions import ObjectDoesNotExist
        try:
            return variables.get(variable_name=variable_name).variable_value
        except ObjectDoesNotExist:
            return ''

    BB_VERSION=_get_variable_or_empty(variable_name='BB_VERSION')
    BUILD_SYS=_get_variable_or_empty(variable_name='BUILD_SYS')
    NATIVELSBSTRING=_get_variable_or_empty(variable_name='NATIVELSBSTRING')
    TARGET_SYS=_get_variable_or_empty(variable_name='TARGET_SYS')
    MACHINE=_get_variable_or_empty(variable_name='MACHINE')
    DISTRO=_get_variable_or_empty(variable_name='DISTRO')
    DISTRO_VERSION=_get_variable_or_empty(variable_name='DISTRO_VERSION')
    TUNE_FEATURES=_get_variable_or_empty(variable_name='TUNE_FEATURES')
    TARGET_FPU=_get_variable_or_empty(variable_name='TARGET_FPU')

    targets = Target.objects.filter(build=build_id)

    context = {
                'objectname': 'configuration',
                'object_search_display':'variables',
                'filter_search_display':'variables',
                'build': Build.objects.get(pk=build_id),
                'BB_VERSION':BB_VERSION,
                'BUILD_SYS':BUILD_SYS,
                'NATIVELSBSTRING':NATIVELSBSTRING,
                'TARGET_SYS':TARGET_SYS,
                'MACHINE':MACHINE,
                'DISTRO':DISTRO,
                'DISTRO_VERSION':DISTRO_VERSION,
                'TUNE_FEATURES':TUNE_FEATURES,
                'TARGET_FPU':TARGET_FPU,
                'targets':targets,
        }
    return render(request, template, context)


def configvars(request, build_id):
    template = 'configvars.html'
    (pagesize, orderby) = _get_parameters_values(request, 100, 'variable_name:+')
    mandatory_parameters = { 'count': pagesize,  'page' : 1, 'orderby' : orderby, 'filter' : 'description__regex:.+' }
    retval = _verify_parameters( request.GET, mandatory_parameters )
    (filter_string, search_term, ordering_string) = _search_tuple(request, Variable)
    if retval:
        # if new search, clear the default filter
        if search_term and len(search_term):
            mandatory_parameters['filter']=''
        return _redirect_parameters( 'configvars', request.GET, mandatory_parameters, build_id = build_id)

    queryset = Variable.objects.filter(build=build_id).exclude(variable_name__istartswith='B_').exclude(variable_name__istartswith='do_')
    queryset_with_search =  _get_queryset(Variable, queryset, None, search_term, ordering_string, 'variable_name').exclude(variable_value='',vhistory__file_name__isnull=True)
    queryset = _get_queryset(Variable, queryset, filter_string, search_term, ordering_string, 'variable_name')
    # remove records where the value is empty AND there are no history files
    queryset = queryset.exclude(variable_value='',vhistory__file_name__isnull=True)

    variables = _build_page_range(Paginator(queryset, pagesize), request.GET.get('page', 1))

    layers = Layer.objects.filter(layer_version_layer__projectlayer__project__build=build_id).order_by("-name")
    layer_names = map(lambda layer : layer.name, layers)
    # special case for meta built-in layer
    layer_names.append('meta')

    # show all matching files (not just the last one)
    file_filter= search_term + ":"
    if filter_string.find('/conf/') > 0:
        file_filter += 'conf/(local|bblayers).conf'
    if filter_string.find('conf/machine/') > 0:
        file_filter += 'conf/machine/'
    if filter_string.find('conf/distro/') > 0:
        file_filter += 'conf/distro/'
    if filter_string.find('/bitbake.conf') > 0:
        file_filter += '/bitbake.conf'
    build_dir=re.sub("/tmp/log/.*","",Build.objects.get(pk=build_id).cooker_log_path)

    context = {
                'objectname': 'configvars',
                'object_search_display':'BitBake variables',
                'filter_search_display':'variables',
                'file_filter': file_filter,
                'build': Build.objects.get(pk=build_id),
                'objects' : variables,
                'total_count':queryset_with_search.count(),
                'default_orderby' : 'variable_name:+',
                'search_term':search_term,
                'layer_names' : layer_names,
            # Specifies the display of columns for the table, appearance in "Edit columns" box, toggling default show/hide, and specifying filters for columns
                'tablecols' : [
                {'name': 'Variable',
                 'qhelp': "BitBake is a generic task executor that considers a list of tasks with dependencies and handles metadata that consists of variables in a certain format that get passed to the tasks",
                 'orderfield': _get_toggle_order(request, "variable_name"),
                 'ordericon':_get_toggle_order_icon(request, "variable_name"),
                },
                {'name': 'Value',
                 'qhelp': "The value assigned to the variable",
                 'dclass': "span4",
                },
                {'name': 'Set in file',
                 'qhelp': "The last configuration file that touched the variable value",
                 'clclass': 'file', 'hidden' : 0,
                 'orderkey' : 'vhistory__file_name',
                 'filter' : {
                    'class' : 'vhistory__file_name',
                    'label': 'Show:',
                    'options' : [
                               ('Local configuration variables', 'vhistory__file_name__contains:'+build_dir+'/conf/',queryset_with_search.filter(vhistory__file_name__contains=build_dir+'/conf/').count(), 'Select this filter to see variables set by the <code>local.conf</code> and <code>bblayers.conf</code> configuration files inside the <code>/build/conf/</code> directory'),
                               ('Machine configuration variables', 'vhistory__file_name__contains:conf/machine/',queryset_with_search.filter(vhistory__file_name__contains='conf/machine').count(), 'Select this filter to see variables set by the configuration file(s) inside your layers <code>/conf/machine/</code> directory'),
                               ('Distro configuration variables', 'vhistory__file_name__contains:conf/distro/',queryset_with_search.filter(vhistory__file_name__contains='conf/distro').count(), 'Select this filter to see variables set by the configuration file(s) inside your layers <code>/conf/distro/</code> directory'),
                               ('Layer configuration variables', 'vhistory__file_name__contains:conf/layer.conf',queryset_with_search.filter(vhistory__file_name__contains='conf/layer.conf').count(), 'Select this filter to see variables set by the <code>layer.conf</code> configuration file inside your layers'),
                               ('bitbake.conf variables', 'vhistory__file_name__contains:/bitbake.conf',queryset_with_search.filter(vhistory__file_name__contains='/bitbake.conf').count(), 'Select this filter to see variables set by the <code>bitbake.conf</code> configuration file'),
                               ]
                             },
                },
                {'name': 'Description',
                 'qhelp': "A brief explanation of the variable",
                 'clclass': 'description', 'hidden' : 0,
                 'dclass': "span4",
                 'filter' : {
                    'class' : 'description',
                    'label': 'Show:',
                    'options' : [
                               ('Variables with description', 'description__regex:.+', queryset_with_search.filter(description__regex='.+').count(), 'We provide descriptions for the most common BitBake variables. The list of descriptions lives in <code>meta/conf/documentation.conf</code>'),
                               ]
                            },
                },
                ],
            }

    response = render(request, template, context)
    _save_parameters_cookies(response, pagesize, orderby, request)
    return response

def bpackage(request, build_id):
    template = 'bpackage.html'
    (pagesize, orderby) = _get_parameters_values(request, 100, 'name:+')
    mandatory_parameters = { 'count' : pagesize,  'page' : 1, 'orderby' : orderby }
    retval = _verify_parameters( request.GET, mandatory_parameters )
    if retval:
        return _redirect_parameters( 'packages', request.GET, mandatory_parameters, build_id = build_id)
    (filter_string, search_term, ordering_string) = _search_tuple(request, Package)
    queryset = Package.objects.filter(build = build_id).filter(size__gte=0)
    queryset = _get_queryset(Package, queryset, filter_string, search_term, ordering_string, 'name')

    packages = _build_page_range(Paginator(queryset, pagesize),request.GET.get('page', 1))

    build = Build.objects.get( pk = build_id )

    context = {
        'objectname': 'packages built',
        'build': build,
        'objects' : packages,
        'default_orderby' : 'name:+',
        'tablecols':[
            {
                'name':'Package',
                'qhelp':'Packaged output resulting from building a recipe',
                'orderfield': _get_toggle_order(request, "name"),
                'ordericon':_get_toggle_order_icon(request, "name"),
            },
            {
                'name':'Package version',
                'qhelp':'The package version and revision',
            },
            {
                'name':'Size',
                'qhelp':'The size of the package',
                'orderfield': _get_toggle_order(request, "size", True),
                'ordericon':_get_toggle_order_icon(request, "size"),
                'orderkey' : 'size',
                'clclass': 'size', 'hidden': 0,
                'dclass' : 'span2',
            },
            {
                'name':'License',
                'qhelp':'The license under which the package is distributed. Multiple license names separated by the pipe character indicates a choice between licenses. Multiple license names separated by the ampersand character indicates multiple licenses exist that cover different parts of the source',
                'orderfield': _get_toggle_order(request, "license"),
                'ordericon':_get_toggle_order_icon(request, "license"),
                'orderkey' : 'license',
                'clclass': 'license', 'hidden': 1,
            },
            {
                'name':'Recipe',
                'qhelp':'The name of the recipe building the package',
                'orderfield': _get_toggle_order(request, "recipe__name"),
                'ordericon':_get_toggle_order_icon(request, "recipe__name"),
                'orderkey' : 'recipe__name',
                'clclass': 'recipe__name', 'hidden': 0,
            },
            {
                'name':'Recipe version',
                'qhelp':'Version and revision of the recipe building the package',
                'clclass': 'recipe__version', 'hidden': 1,
            },
            {
                'name':'Layer',
                'qhelp':'The name of the layer providing the recipe that builds the package',
                'orderfield': _get_toggle_order(request, "recipe__layer_version__layer__name"),
                'ordericon':_get_toggle_order_icon(request, "recipe__layer_version__layer__name"),
                'orderkey' : 'recipe__layer_version__layer__name',
                'clclass': 'recipe__layer_version__layer__name', 'hidden': 1,
            },
            {
                'name':'Layer branch',
                'qhelp':'The Git branch of the layer providing the recipe that builds the package',
                'orderfield': _get_toggle_order(request, "recipe__layer_version__branch"),
                'ordericon':_get_toggle_order_icon(request, "recipe__layer_version__branch"),
                'orderkey' : 'recipe__layer_version__branch',
                'clclass': 'recipe__layer_version__branch', 'hidden': 1,
            },
            {
                'name':'Layer commit',
                'qhelp':'The Git commit of the layer providing the recipe that builds the package',
                'clclass': 'recipe__layer_version__layer__commit', 'hidden': 1,
            },
            ]
        }

    if not toastermain.settings.MANAGED or build.project is None:

        tc_layerDir = {
                'name':'Layer directory',
                'qhelp':'Path to the layer providing the recipe that builds the package',
                'orderfield': _get_toggle_order(request, "recipe__layer_version__layer__local_path"),
                'ordericon':_get_toggle_order_icon(request, "recipe__layer_version__layer__local_path"),
                'orderkey' : 'recipe__layer_version__layer__local_path',
                'clclass': 'recipe__layer_version__layer__local_path', 'hidden': 1,
        }
        context['tablecols'].append(tc_layerDir)

    response = render(request, template, context)
    _save_parameters_cookies(response, pagesize, orderby, request)
    return response

def bfile(request, build_id, package_id):
    template = 'bfile.html'
    files = Package_File.objects.filter(package = package_id)
    context = {'build': Build.objects.get(pk=build_id), 'objects' : files}
    return render(request, template, context)

def tpackage(request, build_id, target_id):
    template = 'package.html'
    packages = map(lambda x: x.package, list(Target_Installed_Package.objects.filter(target=target_id)))
    context = {'build': Build.objects.get(pk=build_id), 'objects' : packages}
    return render(request, template, context)

def layer(request):
    template = 'layer.html'
    layer_info = Layer.objects.all()

    for li in layer_info:
        li.versions = Layer_Version.objects.filter(layer = li)
        for liv in li.versions:
            liv.count = Recipe.objects.filter(layer_version__id = liv.id).count()

    context = {'objects': layer_info}

    return render(request, template, context)


def layer_versions_recipes(request, layerversion_id):
    template = 'recipe.html'
    recipes = Recipe.objects.filter(layer_version__id = layerversion_id)

    context = {'objects': recipes,
            'layer_version' : Layer_Version.objects.get( id = layerversion_id )
    }

    return render(request, template, context)

# A set of dependency types valid for both included and built package views
OTHER_DEPENDS_BASE = [
    Package_Dependency.TYPE_RSUGGESTS,
    Package_Dependency.TYPE_RPROVIDES,
    Package_Dependency.TYPE_RREPLACES,
    Package_Dependency.TYPE_RCONFLICTS,
    ]

# value for invalid row id
INVALID_KEY = -1

"""
Given a package id, target_id retrieves two sets of this image and package's
dependencies.  The return value is a dictionary consisting of two other
lists: a list of 'runtime' dependencies, that is, having RDEPENDS
values in source package's recipe, and a list of other dependencies, that is
the list of possible recipe variables as found in OTHER_DEPENDS_BASE plus
the RRECOMENDS or TRECOMENDS value.
The lists are built in the sort order specified for the package runtime
dependency views.
"""
def _get_package_dependencies(package_id, target_id = INVALID_KEY):
    runtime_deps = []
    other_deps = []
    other_depends_types = OTHER_DEPENDS_BASE

    if target_id != INVALID_KEY :
        rdepends_type = Package_Dependency.TYPE_TRDEPENDS
        other_depends_types +=  [Package_Dependency.TYPE_TRECOMMENDS]
    else :
        rdepends_type = Package_Dependency.TYPE_RDEPENDS
        other_depends_types += [Package_Dependency.TYPE_RRECOMMENDS]

    package = Package.objects.get(pk=package_id)
    if target_id != INVALID_KEY :
        alldeps = package.package_dependencies_source.filter(target_id__exact = target_id)
    else :
        alldeps = package.package_dependencies_source.all()
    for idep in alldeps:
        dep_package = Package.objects.get(pk=idep.depends_on_id)
        dep_entry = Package_Dependency.DEPENDS_DICT[idep.dep_type]
        if dep_package.version == '' :
            version = ''
        else :
            version = dep_package.version + "-" + dep_package.revision
        installed = False
        if target_id != INVALID_KEY :
            if Target_Installed_Package.objects.filter(target_id__exact = target_id, package_id__exact = dep_package.id).count() > 0:
                installed = True
        dep =   {
                'name' : dep_package.name,
                'version' : version,
                'size' : dep_package.size,
                'dep_type' : idep.dep_type,
                'dep_type_display' : dep_entry[0].capitalize(),
                'dep_type_help' : dep_entry[1] % (dep_package.name, package.name),
                'depends_on_id' : dep_package.id,
                'installed' : installed,
                }

        if target_id != INVALID_KEY:
                dep['alias'] = _get_package_alias(dep_package)

        if idep.dep_type == rdepends_type :
            runtime_deps.append(dep)
        elif idep.dep_type in other_depends_types :
            other_deps.append(dep)

    rdep_sorted = sorted(runtime_deps, key=lambda k: k['name'])
    odep_sorted = sorted(
            sorted(other_deps, key=lambda k: k['name']),
            key=lambda k: k['dep_type'])
    retvalues = {'runtime_deps' : rdep_sorted, 'other_deps' : odep_sorted}
    return retvalues

# Return the count of packages dependent on package for this target_id image
def _get_package_reverse_dep_count(package, target_id):
    return package.package_dependencies_target.filter(target_id__exact=target_id, dep_type__exact = Package_Dependency.TYPE_TRDEPENDS).count()

# Return the count of the packages that this package_id is dependent on.
# Use one of the two RDEPENDS types, either TRDEPENDS if the package was
# installed, or else RDEPENDS if only built.
def _get_package_dependency_count(package, target_id, is_installed):
    if is_installed :
        return package.package_dependencies_source.filter(target_id__exact = target_id,
            dep_type__exact = Package_Dependency.TYPE_TRDEPENDS).count()
    else :
        return package.package_dependencies_source.filter(dep_type__exact = Package_Dependency.TYPE_RDEPENDS).count()

def _get_package_alias(package):
    alias = package.installed_name
    if alias != None and alias != '' and alias != package.name:
        return alias
    else:
        return ''

def _get_fullpackagespec(package):
    r = package.name
    version_good = package.version != None and  package.version != ''
    revision_good = package.revision != None and package.revision != ''
    if version_good or revision_good:
        r += '_'
        if version_good:
            r += package.version
            if revision_good:
                r += '-'
        if revision_good:
            r += package.revision
    return r

def package_built_detail(request, build_id, package_id):
    template = "package_built_detail.html"
    if Build.objects.filter(pk=build_id).count() == 0 :
        return redirect(builds)

    # follow convention for pagination w/ search although not used for this view
    queryset = Package_File.objects.filter(package_id__exact=package_id)
    (pagesize, orderby) = _get_parameters_values(request, 25, 'path:+')
    mandatory_parameters = { 'count': pagesize,  'page' : 1, 'orderby' : orderby }
    retval = _verify_parameters( request.GET, mandatory_parameters )
    if retval:
        return _redirect_parameters( 'package_built_detail', request.GET, mandatory_parameters, build_id = build_id, package_id = package_id)

    (filter_string, search_term, ordering_string) = _search_tuple(request, Package_File)
    paths = _get_queryset(Package_File, queryset, filter_string, search_term, ordering_string, 'path')

    package = Package.objects.get(pk=package_id)
    package.fullpackagespec = _get_fullpackagespec(package)
    context = {
            'build' : Build.objects.get(pk=build_id),
            'package' : package,
            'dependency_count' : _get_package_dependency_count(package, -1, False),
            'objects' : paths,
            'tablecols':[
                {
                    'name':'File',
                    'orderfield': _get_toggle_order(request, "path"),
                    'ordericon':_get_toggle_order_icon(request, "path"),
                },
                {
                    'name':'Size',
                    'orderfield': _get_toggle_order(request, "size", True),
                    'ordericon':_get_toggle_order_icon(request, "size"),
                    'dclass': 'sizecol span2',
                },
            ]
    }
    if paths.all().count() < 2:
        context['disable_sort'] = True;

    response = render(request, template, context)
    _save_parameters_cookies(response, pagesize, orderby, request)
    return response

def package_built_dependencies(request, build_id, package_id):
    template = "package_built_dependencies.html"
    if Build.objects.filter(pk=build_id).count() == 0 :
         return redirect(builds)

    package = Package.objects.get(pk=package_id)
    package.fullpackagespec = _get_fullpackagespec(package)
    dependencies = _get_package_dependencies(package_id)
    context = {
            'build' : Build.objects.get(pk=build_id),
            'package' : package,
            'runtime_deps' : dependencies['runtime_deps'],
            'other_deps' :   dependencies['other_deps'],
            'dependency_count' : _get_package_dependency_count(package, -1,  False)
    }
    return render(request, template, context)


def package_included_detail(request, build_id, target_id, package_id):
    template = "package_included_detail.html"
    if Build.objects.filter(pk=build_id).count() == 0 :
        return redirect(builds)

    # follow convention for pagination w/ search although not used for this view
    (pagesize, orderby) = _get_parameters_values(request, 25, 'path:+')
    mandatory_parameters = { 'count': pagesize,  'page' : 1, 'orderby' : orderby }
    retval = _verify_parameters( request.GET, mandatory_parameters )
    if retval:
        return _redirect_parameters( 'package_included_detail', request.GET, mandatory_parameters, build_id = build_id, target_id = target_id, package_id = package_id)
    (filter_string, search_term, ordering_string) = _search_tuple(request, Package_File)

    queryset = Package_File.objects.filter(package_id__exact=package_id)
    paths = _get_queryset(Package_File, queryset, filter_string, search_term, ordering_string, 'path')

    package = Package.objects.get(pk=package_id)
    package.fullpackagespec = _get_fullpackagespec(package)
    package.alias = _get_package_alias(package)
    target = Target.objects.get(pk=target_id)
    context = {
            'build' : Build.objects.get(pk=build_id),
            'target'  : target,
            'package' : package,
            'reverse_count' : _get_package_reverse_dep_count(package, target_id),
            'dependency_count' : _get_package_dependency_count(package, target_id, True),
            'objects': paths,
            'tablecols':[
                {
                    'name':'File',
                    'orderfield': _get_toggle_order(request, "path"),
                    'ordericon':_get_toggle_order_icon(request, "path"),
                },
                {
                    'name':'Size',
                    'orderfield': _get_toggle_order(request, "size", True),
                    'ordericon':_get_toggle_order_icon(request, "size"),
                    'dclass': 'sizecol span2',
                },
            ]
    }
    if paths.all().count() < 2:
        context['disable_sort'] = True
    response = render(request, template, context)
    _save_parameters_cookies(response, pagesize, orderby, request)
    return response

def package_included_dependencies(request, build_id, target_id, package_id):
    template = "package_included_dependencies.html"
    if Build.objects.filter(pk=build_id).count() == 0 :
        return redirect(builds)

    package = Package.objects.get(pk=package_id)
    package.fullpackagespec = _get_fullpackagespec(package)
    package.alias = _get_package_alias(package)
    target = Target.objects.get(pk=target_id)

    dependencies = _get_package_dependencies(package_id, target_id)
    context = {
            'build' : Build.objects.get(pk=build_id),
            'package' : package,
            'target' : target,
            'runtime_deps' : dependencies['runtime_deps'],
            'other_deps' :   dependencies['other_deps'],
            'reverse_count' : _get_package_reverse_dep_count(package, target_id),
            'dependency_count' : _get_package_dependency_count(package, target_id, True)
    }
    return render(request, template, context)

def package_included_reverse_dependencies(request, build_id, target_id, package_id):
    template = "package_included_reverse_dependencies.html"
    if Build.objects.filter(pk=build_id).count() == 0 :
        return redirect(builds)

    (pagesize, orderby) = _get_parameters_values(request, 25, 'package__name:+')
    mandatory_parameters = { 'count': pagesize,  'page' : 1, 'orderby': orderby }
    retval = _verify_parameters( request.GET, mandatory_parameters )
    if retval:
        return _redirect_parameters( 'package_included_reverse_dependencies', request.GET, mandatory_parameters, build_id = build_id, target_id = target_id, package_id = package_id)
    (filter_string, search_term, ordering_string) = _search_tuple(request, Package_File)

    queryset = Package_Dependency.objects.select_related('depends_on__name', 'depends_on__size').filter(depends_on=package_id, target_id=target_id, dep_type=Package_Dependency.TYPE_TRDEPENDS)
    objects = _get_queryset(Package_Dependency, queryset, filter_string, search_term, ordering_string, 'package__name')

    package = Package.objects.get(pk=package_id)
    package.fullpackagespec = _get_fullpackagespec(package)
    package.alias = _get_package_alias(package)
    target = Target.objects.get(pk=target_id)
    for o in objects:
        if o.package.version != '':
            o.package.version += '-' + o.package.revision
        o.alias = _get_package_alias(o.package)
    context = {
            'build' : Build.objects.get(pk=build_id),
            'package' : package,
            'target' : target,
            'objects' : objects,
            'reverse_count' : _get_package_reverse_dep_count(package, target_id),
            'dependency_count' : _get_package_dependency_count(package, target_id, True),
            'tablecols':[
                {
                    'name':'Package',
                    'orderfield': _get_toggle_order(request, "package__name"),
                    'ordericon': _get_toggle_order_icon(request, "package__name"),
                },
                {
                    'name':'Version',
                },
                {
                    'name':'Size',
                    'orderfield': _get_toggle_order(request, "package__size", True),
                    'ordericon': _get_toggle_order_icon(request, "package__size"),
                    'dclass': 'sizecol span2',
                },
            ]
    }
    if objects.all().count() < 2:
        context['disable_sort'] = True
    response = render(request, template, context)
    _save_parameters_cookies(response, pagesize, orderby, request)
    return response

def image_information_dir(request, build_id, target_id, packagefile_id):
    # stubbed for now
    return redirect(builds)


import toastermain.settings


# we have a set of functions if we're in managed mode, or
# a default "page not available" simple functions for interactive mode
if toastermain.settings.MANAGED:

    from django.contrib.auth.models import User
    from django.contrib.auth import authenticate, login
    from django.contrib.auth.decorators import login_required

    from orm.models import Project, ProjectLayer, ProjectTarget, ProjectVariable
    from orm.models import Branch, LayerSource, ToasterSetting, Release, Machine, LayerVersionDependency
    from bldcontrol.models import BuildRequest

    import traceback

    class BadParameterException(Exception): pass        # error thrown on invalid POST requests

    # the context processor that supplies data used across all the pages
    def managedcontextprocessor(request):
        import subprocess
        ret = {
            "projects": Project.objects.all(),
            "MANAGED" : toastermain.settings.MANAGED,
            "DEBUG" : toastermain.settings.DEBUG,
            "TOASTER_BRANCH": toastermain.settings.TOASTER_BRANCH,
            "TOASTER_REVISION" : toastermain.settings.TOASTER_REVISION,
        }
        if 'project_id' in request.session:
            try:
                ret['project'] = Project.objects.get(pk = request.session['project_id'])
            except Project.DoesNotExist:
                del request.session['project_id']
        return ret


    class InvalidRequestException(Exception):
        def __init__(self, response):
            self.response = response


    # shows the "all builds" page for managed mode; it displays build requests (at least started!) instead of actual builds
    def builds(request):
        template = 'managed_builds.html'
        # define here what parameters the view needs in the GET portion in order to
        # be able to display something.  'count' and 'page' are mandatory for all views
        # that use paginators.

        buildrequests = BuildRequest.objects.exclude(state__lte = BuildRequest.REQ_INPROGRESS).exclude(state=BuildRequest.REQ_DELETED)

        try:
            context, pagesize, orderby = _build_list_helper(request, buildrequests, True)
        except InvalidRequestException as e:
            return _redirect_parameters( builds, request.GET, e.response)

        response = render(request, template, context)
        _save_parameters_cookies(response, pagesize, orderby, request)
        return response



    # helper function, to be used on "all builds" and "project builds" pages
    def _build_list_helper(request, buildrequests, insert_projects):
        # ATTN: we use here the ordering parameters for interactive mode; the translation for BuildRequest fields will happen below
        default_orderby = 'completed_on:-'
        (pagesize, orderby) = _get_parameters_values(request, 10, default_orderby)
        mandatory_parameters = { 'count': pagesize,  'page' : 1, 'orderby' : orderby }
        retval = _verify_parameters( request.GET, mandatory_parameters )
        if retval:
            raise InvalidRequestException(mandatory_parameters)

        orig_orderby = orderby
        # translate interactive mode ordering to managed mode ordering
        ordering_params = orderby.split(":")
        if ordering_params[0] == "completed_on":
            ordering_params[0] = "updated"
        if ordering_params[0] == "started_on":
            ordering_params[0] = "created"
        if ordering_params[0] == "errors_no":
            ordering_params[0] = "build__errors_no"
        if ordering_params[0] == "warnings_no":
            ordering_params[0] = "build__warnings_no"
        if ordering_params[0] == "machine":
            ordering_params[0] = "build__machine"
        if ordering_params[0] == "target__target":
            ordering_params[0] = "brtarget__target"
        if ordering_params[0] == "timespent":
            ordering_params[0] = "id"
            orderby = default_orderby

        request.GET = request.GET.copy()        # get a mutable copy of the GET QueryDict
        request.GET['orderby'] = ":".join(ordering_params)

        # boilerplate code that takes a request for an object type and returns a queryset
        # for that object type. copypasta for all needed table searches
        (filter_string, search_term, ordering_string) = _search_tuple(request, BuildRequest)
        # post-process any date range filters
        filter_string,daterange_selected = _modify_date_range_filter(filter_string)

        # we don't display in-progress or deleted builds
        queryset_all = buildrequests.exclude(state = BuildRequest.REQ_DELETED)
        queryset_all = queryset_all.select_related("build", "build__project").annotate(Count('brerror'))
        queryset_with_search = _get_queryset(BuildRequest, queryset_all, filter_string, search_term, ordering_string, '-updated')


        # retrieve the objects that will be displayed in the table; builds a paginator and gets a page range to display
        build_info = _build_page_range(Paginator(queryset_with_search, pagesize), request.GET.get('page', 1))

        # build view-specific information; this is rendered specifically in the builds page, at the top of the page (i.e. Recent builds)
        # most recent build is like projects' most recent builds, but across all projects
        build_mru = _managed_get_latest_builds()

        fstypes_map = {};
        for build_request in build_info:
            # set display variables for build request
            build_request.machine = build_request.brvariable_set.get(name="MACHINE").value
            build_request.timespent = build_request.updated - build_request.created

            # set up list of fstypes for each build
            if build_request.build is None:
                continue
            targets = Target.objects.filter( build_id = build_request.build.id )
            comma = "";
            extensions = "";
            for t in targets:
                if ( not t.is_image ):
                    continue
                tif = Target_Image_File.objects.filter( target_id = t.id )
                for i in tif:
                    s=re.sub('.*tar.bz2', 'tar.bz2', i.file_name)
                    if s == i.file_name:
                        s=re.sub('.*\.', '', i.file_name)
                    if None == re.search(s,extensions):
                        extensions += comma + s
                        comma = ", "
            fstypes_map[build_request.build.id]=extensions


        # send the data to the template
        context = {
                # specific info for
                    'mru' : build_mru,
                # TODO: common objects for all table views, adapt as needed
                    'objects' : build_info,
                    'objectname' : "builds",
                    'default_orderby' : 'updated:-',
                    'fstypes' : fstypes_map,
                    'search_term' : search_term,
                    'total_count' : queryset_with_search.count(),
                    'daterange_selected' : daterange_selected,
                # Specifies the display of columns for the table, appearance in "Edit columns" box, toggling default show/hide, and specifying filters for columns
                    'tablecols' : [
                    {'name': 'Outcome',                                                # column with a single filter
                     'qhelp' : "The outcome tells you if a build successfully completed or failed",     # the help button content
                     'dclass' : "span2",                                                # indication about column width; comes from the design
                     'orderfield': _get_toggle_order(request, "state"),               # adds ordering by the field value; default ascending unless clicked from ascending into descending
                     'ordericon':_get_toggle_order_icon(request, "state"),
                      # filter field will set a filter on that column with the specs in the filter description
                      # the class field in the filter has no relation with clclass; the control different aspects of the UI
                      # still, it is recommended for the values to be identical for easy tracking in the generated HTML
                     'filter' : {'class' : 'outcome',
                                 'label': 'Show:',
                                 'options' : [
                                             ('Successful builds', 'build__outcome:' + str(Build.SUCCEEDED), queryset_all.filter(build__outcome = Build.SUCCEEDED).count()),  # this is the field search expression
                                             ('Failed builds', 'build__outcome:NOT'+ str(Build.SUCCEEDED), queryset_all.exclude(build__outcome = Build.SUCCEEDED).count()),
                                             ]
                                }
                    },
                    {'name': 'Recipe',                                                 # default column, disabled box, with just the name in the list
                     'qhelp': "What you built (i.e. one or more recipes or image recipes)",
                     'orderfield': _get_toggle_order(request, "brtarget__target"),
                     'ordericon':_get_toggle_order_icon(request, "brtarget__target"),
                    },
                    {'name': 'Machine',
                     'qhelp': "The machine is the hardware for which you are building a recipe or image recipe",
                     'orderfield': _get_toggle_order(request, "build__machine"),
                     'ordericon':_get_toggle_order_icon(request, "build__machine"),
                     'dclass': 'span3'
                    },                           # a slightly wider column
                    ]
                }

        if (insert_projects):
            context['tablecols'].append(
                    {'name': 'Project', 'clclass': 'project',
                    }
            )

        # calculate the exact begining of local today and yesterday
        context_date,today_begin,yesterday_begin = _add_daterange_context(queryset_all, request, {'created','updated'})
        context.update(context_date)

        context['tablecols'].append(
                    {'name': 'Started on', 'clclass': 'started_on', 'hidden' : 1,      # this is an unchecked box, which hides the column
                     'qhelp': "The date and time you started the build",
                     'orderfield': _get_toggle_order(request, "created", True),
                     'ordericon':_get_toggle_order_icon(request, "created"),
                     'filter' : {'class' : 'created',
                                 'label': 'Show:',
                                 'options' : [
                                             ("Today's builds" , 'created__gte:'+today_begin.strftime("%Y-%m-%d"), queryset_all.filter(created__gte=today_begin).count()),
                                             ("Yesterday's builds",
                                                 'created__gte!created__lt:'
                                                     +yesterday_begin.strftime("%Y-%m-%d")+'!'
                                                     +today_begin.strftime("%Y-%m-%d"),
                                                 queryset_all.filter(
                                                     created__gte=yesterday_begin,
                                                     created__lt=today_begin
                                                     ).count()),
                                             ("Build date range", 'daterange', 1, '', 'created'),
                                             ]
                                }
                    }
            )
        context['tablecols'].append(
                    {'name': 'Completed on',
                     'qhelp': "The date and time the build finished",
                     'orderfield': _get_toggle_order(request, "updated", True),
                     'ordericon':_get_toggle_order_icon(request, "updated"),
                     'orderkey' : 'updated',
                     'filter' : {'class' : 'updated',
                                 'label': 'Show:',
                                 'options' : [
                                             ("Today's builds" , 'updated__gte:'+today_begin.strftime("%Y-%m-%d"), queryset_all.filter(updated__gte=today_begin).count()),
                                             ("Yesterday's builds",
                                                 'updated__gte!updated__lt:'
                                                     +yesterday_begin.strftime("%Y-%m-%d")+'!'
                                                     +today_begin.strftime("%Y-%m-%d"),
                                                 queryset_all.filter(
                                                     updated__gte=yesterday_begin,
                                                     updated__lt=today_begin
                                                     ).count()),
                                             ("Build date range", 'daterange', 1, '', 'updated'),
                                             ]
                                }
                    }
            )
        context['tablecols'].append(
                    {'name': 'Failed tasks', 'clclass': 'failed_tasks',                # specifing a clclass will enable the checkbox
                     'qhelp': "How many tasks failed during the build",
                     'filter' : {'class' : 'failed_tasks',
                                 'label': 'Show:',
                                 'options' : [
                                             ('Builds with failed tasks', 'build__task_build__outcome:%d' % Task.OUTCOME_FAILED,
                                                queryset_all.filter(build__task_build__outcome=Task.OUTCOME_FAILED).count()),
                                             ('Builds without failed tasks', 'build__task_build__outcome:%d' % Task.OUTCOME_FAILED,
                                                queryset_all.filter(~Q(build__task_build__outcome=Task.OUTCOME_FAILED)).count()),
                                             ]
                                }
                    }
            )
        context['tablecols'].append(
                    {'name': 'Errors', 'clclass': 'errors_no',
                     'qhelp': "How many errors were encountered during the build (if any)",
                     'orderfield': _get_toggle_order(request, "build__errors_no", True),
                     'ordericon':_get_toggle_order_icon(request, "build__errors_no"),
                     'orderkey' : 'errors_no',
                     'filter' : {'class' : 'errors_no',
                                 'label': 'Show:',
                                 'options' : [
                                             ('Builds with errors', 'build|build__errors_no__gt:None|0',
                                                queryset_all.filter(Q(build=None) | Q(build__errors_no__gt=0)).count()),
                                             ('Builds without errors', 'build__errors_no:0',
                                                queryset_all.filter(build__errors_no=0).count()),
                                             ]
                                }
                    }
            )
        context['tablecols'].append(
                    {'name': 'Warnings', 'clclass': 'warnings_no',
                     'qhelp': "How many warnings were encountered during the build (if any)",
                     'orderfield': _get_toggle_order(request, "build__warnings_no", True),
                     'ordericon':_get_toggle_order_icon(request, "build__warnings_no"),
                     'orderkey' : 'build__warnings_no',
                     'filter' : {'class' : 'build__warnings_no',
                                 'label': 'Show:',
                                 'options' : [
                                             ('Builds with warnings','build__warnings_no__gte:1', queryset_all.filter(build__warnings_no__gte=1).count()),
                                             ('Builds without warnings','build__warnings_no:0', queryset_all.filter(build__warnings_no=0).count()),
                                             ]
                                }
                    }
            )
        context['tablecols'].append(
                    {'name': 'Time', 'clclass': 'time', 'hidden' : 1,
                     'qhelp': "How long it took the build to finish",
#                    'orderfield': _get_toggle_order(request, "timespent", True),
#                    'ordericon':_get_toggle_order_icon(request, "timespent"),
                     'orderkey' : 'timespent',
                    }
            )
        context['tablecols'].append(
                    {'name': 'Image files', 'clclass': 'output',
                     'qhelp': "The root file system types produced by the build. You can find them in your <code>/build/tmp/deploy/images/</code> directory",
                        # TODO: compute image fstypes from Target_Image_File
                    }
            )

        return context, pagesize, orderby

    # new project
    def newproject(request):
        template = "newproject.html"
        context = {
            'email': request.user.email if request.user.is_authenticated() else '',
            'username': request.user.username if request.user.is_authenticated() else '',
            'releases': Release.objects.order_by("description"),
        }

        try:
            context['defaultbranch'] = ToasterSetting.objects.get(name = "DEFAULT_RELEASE").value
        except ToasterSetting.DoesNotExist:
            pass

        if request.method == "GET":
            # render new project page
            return render(request, template, context)
        elif request.method == "POST":
            mandatory_fields = ['projectname', 'projectversion']
            try:
                # make sure we have values for all mandatory_fields
                if reduce( lambda x, y: x or y, map(lambda x: len(request.POST.get(x, '')) == 0, mandatory_fields)):
                # set alert for missing fields
                    raise BadParameterException("Fields missing: " +
            ", ".join([x for x in mandatory_fields if len(request.POST.get(x, '')) == 0 ]))

                if not request.user.is_authenticated():
                    user = authenticate(username = request.POST.get('username', '_anonuser'), password = 'nopass')
                    if user is None:
                        user = User.objects.create_user(username = request.POST.get('username', '_anonuser'), email = request.POST.get('email', ''), password = "nopass")

                        user = authenticate(username = user.username, password = 'nopass')
                    login(request, user)

                #  save the project
                prj = Project.objects.create_project(name = request.POST['projectname'], release = Release.objects.get(pk = request.POST['projectversion']))
                prj.user_id = request.user.pk
                prj.save()
                return redirect(reverse(project, args=(prj.pk,)) + "#/newproject")

            except (IntegrityError, BadParameterException) as e:
                # fill in page with previously submitted values
                map(lambda x: context.__setitem__(x, request.POST.get(x, "-- missing")), mandatory_fields)
                if isinstance(e, IntegrityError) and "username" in str(e):
                    context['alert'] = "Your chosen username is already used"
                else:
                    context['alert'] = str(e)
                return render(request, template, context)

        raise Exception("Invalid HTTP method for this page")



    # Shows the edit project page
    def project(request, pid):
        template = "project.html"
        try:
            prj = Project.objects.get(id = pid)
        except Project.DoesNotExist:
            return HttpResponseNotFound("<h1>Project id " + pid + " is unavailable</h1>")

        try:
            puser = User.objects.get(id = prj.user_id)
        except User.DoesNotExist:
            puser = None

        # we use implicit knowledge of the current user's project to filter layer information, e.g.
        request.session['project_id'] = prj.id

        from collections import Counter
        freqtargets = []
        try:
            freqtargets += map(lambda x: x.target, reduce(lambda x, y: x + y,   map(lambda x: list(x.target_set.all()), Build.objects.filter(project = prj, outcome__lt = Build.IN_PROGRESS))))
            freqtargets += map(lambda x: x.target, reduce(lambda x, y: x + y,   map(lambda x: list(x.brtarget_set.all()), BuildRequest.objects.filter(project = prj, state = BuildRequest.REQ_FAILED))))
        except TypeError:
            pass
        freqtargets = Counter(freqtargets)
        freqtargets = sorted(freqtargets, key = lambda x: freqtargets[x], reverse=True)

        context = {
            "project" : prj,
            "lvs_nos" : Layer_Version.objects.all().count(),
            "completedbuilds": BuildRequest.objects.filter(project_id = pid).exclude(state__lte = BuildRequest.REQ_INPROGRESS).exclude(state=BuildRequest.REQ_DELETED),
            "prj" : {"name": prj.name, "release": { "id": prj.release.pk, "name": prj.release.name, "desc": prj.release.description}},
            #"buildrequests" : prj.buildrequest_set.filter(state=BuildRequest.REQ_QUEUED),
            "builds" : _project_recent_build_list(prj),
            "layers" :  map(lambda x: {
                        "id": x.layercommit.pk,
                        "orderid": x.pk,
                        "name" : x.layercommit.layer.name,
                        "giturl": x.layercommit.layer.vcs_url,
                        "url": x.layercommit.layer.layer_index_url,
                        "layerdetailurl": reverse("layerdetails", args=(x.layercommit.pk,)),
                # This branch name is actually the release
                        "branch" : { "name" : x.layercommit.get_vcs_reference(), "layersource" : x.layercommit.up_branch.layer_source.name if x.layercommit.up_branch != None else None}},
                    prj.projectlayer_set.all().order_by("id")),
            "targets" : map(lambda x: {"target" : x.target, "task" : x.task, "pk": x.pk}, prj.projecttarget_set.all()),
            "freqtargets": freqtargets[:5],
            "releases": map(lambda x: {"id": x.pk, "name": x.name, "description":x.description}, Release.objects.all()),
            "project_html": 1,
        }
        try:
            context["machine"] = {"name": prj.projectvariable_set.get(name="MACHINE").value}
        except ProjectVariable.DoesNotExist:
            context["machine"] = None
        try:
            context["distro"] = prj.projectvariable_set.get(name="DISTRO").value
        except ProjectVariable.DoesNotExist:
            context["distro"] = "-- not set yet"

        response = render(request, template, context)
        response['Cache-Control'] = "no-cache, must-revalidate, no-store"
        response['Pragma'] = "no-cache"
        return response

    # This is a wrapper for xhr_projectbuild which allows for a project id
    # which only becomes known client side.
    def xhr_build(request):
        if request.POST.has_key("project_id"):
            pid = request.POST['project_id']
            return xhr_projectbuild(request, pid)
        else:
            raise BadParameterException("invalid project id")

    def xhr_projectbuild(request, pid):
        try:
            if request.method != "POST":
                raise BadParameterException("invalid method")
            request.session['project_id'] = pid
            prj = Project.objects.get(id = pid)


            if 'buildCancel' in request.POST:
                for i in request.POST['buildCancel'].strip().split(" "):
                    try:
                        br = BuildRequest.objects.select_for_update().get(project = prj, pk = i, state__lte = BuildRequest.REQ_QUEUED)
                        br.state = BuildRequest.REQ_DELETED
                        br.save()
                    except BuildRequest.DoesNotExist:
                        pass

            if 'buildDelete' in request.POST:
                for i in request.POST['buildDelete'].strip().split(" "):
                    try:
                        br = BuildRequest.objects.select_for_update().get(project = prj, pk = i, state__lte = BuildRequest.REQ_DELETED).delete()
                    except BuildRequest.DoesNotExist:
                        pass

            if 'targets' in request.POST:
                ProjectTarget.objects.filter(project = prj).delete()
                s = str(request.POST['targets'])
                for t in s.translate(None, ";%|\"").split(" "):
                    if ":" in t:
                        target, task = t.split(":")
                    else:
                        target = t
                        task = ""
                    ProjectTarget.objects.create(project = prj, target = target, task = task)

                br = prj.schedule_build()

            return HttpResponse(jsonfilter({"error":"ok",
                "builds" : _project_recent_build_list(prj),
            }), content_type = "application/json")
        except Exception as e:
            return HttpResponse(jsonfilter({"error":str(e) + "\n" + traceback.format_exc()}), content_type = "application/json")

    # This is a wraper for xhr_projectedit which allows for a project id
    # which only becomes known client side
    def xhr_projectinfo(request):
        if request.POST.has_key("project_id") == False:
            raise BadParameterException("invalid project id")

        return xhr_projectedit(request, request.POST['project_id'])

    def xhr_projectedit(request, pid):
        try:
            prj = Project.objects.get(id = pid)
            # add layers
            if 'layerAdd' in request.POST:
                for lc in Layer_Version.objects.filter(pk__in=request.POST['layerAdd'].split(",")):
                    ProjectLayer.objects.get_or_create(project = prj, layercommit = lc)

            # remove layers
            if 'layerDel' in request.POST:
                for t in request.POST['layerDel'].strip().split(" "):
                    pt = ProjectLayer.objects.filter(project = prj, layercommit_id = int(t)).delete()

            if 'projectName' in request.POST:
                prj.name = request.POST['projectName']
                prj.save();

            if 'projectVersion' in request.POST:
                prj.release = Release.objects.get(pk = request.POST['projectVersion'])
                # we need to change the bitbake version
                prj.bitbake_version = prj.release.bitbake_version
                prj.save()
                # we need to change the layers
                for i in prj.projectlayer_set.all():
                    # find and add a similarly-named layer on the new branch
                    try:
                        lv = prj.compatible_layerversions(layer_name = i.layercommit.layer.name)[0]
                        ProjectLayer.objects.get_or_create(project = prj, layercommit = lv)
                    except IndexError:
                        pass
                    finally:
                        # get rid of the old entry
                        i.delete()

            if 'machineName' in request.POST:
                machinevar = prj.projectvariable_set.get(name="MACHINE")
                machinevar.value=request.POST['machineName']
                machinevar.save()

            # return all project settings
            return HttpResponse(jsonfilter( {
                "error": "ok",
                "layers" :  map(lambda x: {"id": x.layercommit.pk, "orderid" : x.pk, "name" : x.layercommit.layer.name, "giturl" : x.layercommit.layer.vcs_url, "url": x.layercommit.layer.layer_index_url, "layerdetailurl": reverse("layerdetails", args=(x.layercommit.pk,)), "branch" : { "name" : x.layercommit.get_vcs_reference(), "layersource" : x.layercommit.up_branch.layer_source.name}}, prj.projectlayer_set.all().select_related("layer").order_by("id")),
                "builds" : _project_recent_build_list(prj),
                "variables": map(lambda x: (x.name, x.value), prj.projectvariable_set.all()),
                "machine": {"name": prj.projectvariable_set.get(name="MACHINE").value},
                "prj": {"name": prj.name, "release": { "id": prj.release.pk, "name": prj.release.name, "desc": prj.release.description}},
                }), content_type = "application/json")

        except Exception as e:
            return HttpResponse(jsonfilter({"error":str(e) + "\n" + traceback.format_exc()}), content_type = "application/json")


    from django.views.decorators.csrf import csrf_exempt
    @csrf_exempt
    def xhr_datatypeahead(request):
        try:
            prj = None
            if request.GET.has_key('project_id'):
                prj = Project.objects.get(pk = request.GET['project_id'])
            elif 'project_id' in request.session:
                prj = Project.objects.get(pk = request.session['project_id'])
            else:
                raise Exception("No valid project selected")


            def _lv_to_dict(x):
                return {"id": x.pk,
                        "name": x.layer.name,
                        "tooltip": x.layer.vcs_url+" | "+x.get_vcs_reference(),
                        "detail": "(" + x.layer.vcs_url + (")" if x.up_branch == None else " | "+x.get_vcs_reference()+")"),
                        "giturl": x.layer.vcs_url,
                        "layerdetailurl" : reverse('layerdetails', args=(x.pk,)),
                        "revision" : x.get_vcs_reference(),
                       }


            # returns layers for current project release that are not in the project set, matching the name
            if request.GET['type'] == "layers":
                # all layers for the current project
                queryset_all = prj.compatible_layerversions().filter(layer__name__icontains=request.GET.get('value',''))

                # but not layers with equivalent layers already in project
                if not request.GET.has_key('include_added'):
                    queryset_all = queryset_all.exclude(pk__in = [x.id for x in prj.projectlayer_equivalent_set()])[:8]

                # and show only the selected layers for this project
                final_list = set([x.get_equivalents_wpriority(prj)[0] for x in queryset_all])

                return HttpResponse(jsonfilter( { "error":"ok",  "list" : map( _lv_to_dict, sorted(final_list, key = lambda x: x.layer.name)) }), content_type = "application/json")


            # returns layer dependencies for a layer, excluding current project layers
            if request.GET['type'] == "layerdeps":
                queryset = prj.compatible_layerversions().exclude(pk__in = [x.id for x in prj.projectlayer_equivalent_set()]).filter(
                    layer__name__in = [ x.depends_on.layer.name for x in LayerVersionDependency.objects.filter(layer_version_id = request.GET['value'])])

                final_list = set([x.get_equivalents_wpriority(prj)[0] for x in queryset])

                return HttpResponse(jsonfilter( { "error":"ok", "list" : map( _lv_to_dict, sorted(final_list, key = lambda x: x.layer.name)) }), content_type = "application/json")



            # returns layer versions that would be deleted on the new release__pk
            if request.GET['type'] == "versionlayers":
                if not 'project_id' in request.session:
                    raise Exception("This call cannot makes no sense outside a project context")

                retval = []
                for i in prj.projectlayer_set.all():
                    lv = prj.compatible_layerversions(release = Release.objects.get(pk=request.GET['value'])).filter(layer__name = i.layercommit.layer.name)
                    # there is no layer_version with the new release id, and the same name
                    if lv.count() < 1:
                        retval.append(i)

                return HttpResponse(jsonfilter( {"error":"ok",
                    "list" : map( _lv_to_dict,  map(lambda x: x.layercommit, retval ))
                    }), content_type = "application/json")


            # returns layer versions that provide the named targets
            if request.GET['type'] == "layers4target":
                # we return data only if the recipe can't be provided by the current project layer set
                if reduce(lambda x, y: x + y, [x.recipe_layer_version.filter(name=request.GET['value']).count() for x in prj.projectlayer_equivalent_set()], 0):
                    final_list = []
                else:
                    queryset_all = prj.compatible_layerversions().filter(recipe_layer_version__name = request.GET['value'])

                    # exclude layers in the project
                    queryset_all = queryset_all.exclude(pk__in = [x.id for x in prj.projectlayer_equivalent_set()])

                    # and show only the selected layers for this project
                    final_list = set([x.get_equivalents_wpriority(prj)[0] for x in queryset_all])

                return HttpResponse(jsonfilter( { "error":"ok",  "list" : map( _lv_to_dict, final_list) }), content_type = "application/json")

            # returns targets provided by current project layers
            if request.GET['type'] == "targets":
                search_token = request.GET.get('value','')
                queryset_all = Recipe.objects.filter(layer_version__layer__name__in = [x.layercommit.layer.name for x in prj.projectlayer_set.all().select_related("layercommit__layer")]).filter(Q(name__icontains=search_token) | Q(layer_version__layer__name__icontains=search_token))

#                layer_equivalent_set = []
#                for i in prj.projectlayer_set.all().select_related("layercommit__up_branch", "layercommit__layer"):
#                    layer_equivalent_set += i.layercommit.get_equivalents_wpriority(prj)

#                queryset_all = queryset_all.filter(layer_version__in =  layer_equivalent_set)

                # if we have more than one hit here (for distinct name and version), max the id it out
                queryset_all_maxids = queryset_all.values('name').distinct().annotate(max_id=Max('id')).values_list('max_id')
                queryset_all = queryset_all.filter(id__in = queryset_all_maxids).order_by("name").select_related("layer_version__layer")


                return HttpResponse(jsonfilter({ "error":"ok",
                    "list" :
                        # 7152 - sort by token position
                        sorted (
                            map ( lambda x: {"id": x.pk, "name": x.name, "detail":"[" + x.layer_version.layer.name +"]"},
                        queryset_all[:8]),
                            key = lambda i: i["name"].find(search_token) if i["name"].find(search_token) > -1 else 9999,
                        )

                    }), content_type = "application/json")

            # returns machines provided by the current project layers
            if request.GET['type'] == "machines":
                queryset_all = Machine.objects.all()
                if 'project_id' in request.session:
                    queryset_all = queryset_all.filter(layer_version__in =  prj.projectlayer_equivalent_set()).order_by("name")

                search_token = request.GET.get('value','')
                queryset_all = queryset_all.filter(Q(name__icontains=search_token) | Q(description__icontains=search_token))

                return HttpResponse(jsonfilter({ "error":"ok",
                        "list" :
                        # 7152 - sort by the token position
                        sorted (
                            map ( lambda x: {"id": x.pk, "name": x.name, "detail":"[" + x.layer_version.layer.name+ "]"}, queryset_all[:8]),
                            key = lambda i: i["name"].find(search_token) if i["name"].find(search_token) > -1 else 9999,
                        )
                    }), content_type = "application/json")

            # returns all projects
            if request.GET['type'] == "projects":
                queryset_all = Project.objects.all()
                ret = { "error": "ok",
                       "list": map (lambda x: {"id":x.pk, "name": x.name},
                                    queryset_all.filter(name__icontains=request.GET.get('value',''))[:8])}

                return HttpResponse(jsonfilter(ret), content_type = "application/json")

            raise Exception("Unknown request! " + request.GET.get('type', "No parameter supplied"))
        except Exception as e:
            return HttpResponse(jsonfilter({"error":str(e) + "\n" + traceback.format_exc()}), content_type = "application/json")


    def xhr_configvaredit(request, pid):
        try:
            prj = Project.objects.get(id = pid)
            # add conf variables
            if 'configvarAdd' in request.POST:
                t=request.POST['configvarAdd'].strip()
                if ":" in t:
                    variable, value = t.split(":")
                else:
                    variable = t
                    value = ""

                pt, created = ProjectVariable.objects.get_or_create(project = prj, name = variable, value = value)
            # change conf variables
            if 'configvarChange' in request.POST:
                t=request.POST['configvarChange'].strip()
                if ":" in t:
                    variable, value = t.split(":")
                else:
                    variable = t
                    value = ""

                pt, created = ProjectVariable.objects.get_or_create(project = prj, name = variable)
                pt.value=value
                pt.save()
            # remove conf variables
            if 'configvarDel' in request.POST:
                t=request.POST['configvarDel'].strip()
                pt = ProjectVariable.objects.get(pk = int(t)).delete()

            # return all project settings, filter out blacklist and elsewhere-managed variables
            vars_managed,vars_fstypes,vars_blacklist = get_project_configvars_context()
            configvars_query = ProjectVariable.objects.filter(project_id = pid).all()
            for var in vars_managed:
                configvars_query = configvars_query.exclude(name = var)
            for var in vars_blacklist:
                configvars_query = configvars_query.exclude(name = var)

            return_data = {
                "error": "ok",
                'configvars'   : map(lambda x: (x.name, x.value, x.pk), configvars_query),
               }
            try:
                return_data['distro'] = ProjectVariable.objects.get(project = prj, name = "DISTRO").value,
            except ProjectVariable.DoesNotExist:
                pass
            try:
                return_data['fstypes'] = ProjectVariable.objects.get(project = prj, name = "IMAGE_FSTYPES").value,
            except ProjectVariable.DoesNotExist:
                pass
            try:
                return_data['image_install_append'] = ProjectVariable.objects.get(project = prj, name = "IMAGE_INSTALL_append").value,
            except ProjectVariable.DoesNotExist:
                pass
            try:
                return_data['package_classes'] = ProjectVariable.objects.get(project = prj, name = "PACKAGE_CLASSES").value,
            except ProjectVariable.DoesNotExist:
                pass
            try:
                return_data['sdk_machine'] = ProjectVariable.objects.get(project = prj, name = "SDKMACHINE").value,
            except ProjectVariable.DoesNotExist:
                pass

            return HttpResponse(json.dumps( return_data ), content_type = "application/json")

        except Exception as e:
            return HttpResponse(json.dumps({"error":str(e) + "\n" + traceback.format_exc()}), content_type = "application/json")


    def xhr_importlayer(request):
        if (not request.POST.has_key('vcs_url') or
            not request.POST.has_key('name') or
            not request.POST.has_key('git_ref') or
            not request.POST.has_key('project_id')):
          return HttpResponse(jsonfilter({"error": "Missing parameters; requires vcs_url, name, git_ref and project_id"}), content_type = "application/json")

        layers_added = [];

        # Rudimentary check for any possible html tags
        if "<" in request.POST:
          return HttpResponse(jsonfilter({"error": "Invalid character <"}), content_type = "application/json")

        prj = Project.objects.get(pk=request.POST['project_id'])

        # Strip trailing/leading whitespace from all values
        # put into a new dict because POST one is immutable
        post_data = dict()
        for key,val in request.POST.iteritems():
          post_data[key] = val.strip()


        # We need to know what release the current project is so that we
        # can set the imported layer's up_branch_id
        prj_branch_name = Release.objects.get(pk=prj.release_id).branch_name
        up_branch, branch_created = Branch.objects.get_or_create(name=prj_branch_name, layer_source_id=LayerSource.TYPE_IMPORTED)

        layer_source = LayerSource.objects.get(sourcetype=LayerSource.TYPE_IMPORTED)
        try:
            layer, layer_created = Layer.objects.get_or_create(name=post_data['name'])
        except MultipleObjectsReturned:
            return HttpResponse(jsonfilter({"error": "hint-layer-exists"}), content_type = "application/json")

        if layer:
            if layer_created:
                layer.layer_source = layer_source
                layer.vcs_url = post_data['vcs_url']
                layer.up_date = timezone.now()
                layer.save()
            else:
                # We have an existing layer by this name, let's see if the git
                # url is the same, if it is then we can just create a new layer
                # version for this layer. Otherwise we need to bail out.
                if layer.vcs_url != post_data['vcs_url']:
                    return HttpResponse(jsonfilter({"error": "hint-layer-exists-with-different-url" , "current_url" : layer.vcs_url, "current_id": layer.id }), content_type = "application/json")


            layer_version, version_created = Layer_Version.objects.get_or_create(layer_source=layer_source, layer=layer, project=prj, up_branch_id=up_branch.id,branch=post_data['git_ref'],  commit=post_data['git_ref'], dirpath=post_data['dir_path'])

            if layer_version:
                if not version_created:
                    return HttpResponse(jsonfilter({"error": "hint-layer-version-exists", "existing_layer_version": layer_version.id }), content_type = "application/json")

                layer_version.up_date = timezone.now()
                layer_version.save()

                # Add the dependencies specified for this new layer
                if (post_data.has_key("layer_deps") and
                    version_created and
                    len(post_data["layer_deps"]) > 0):
                    for layer_dep_id in post_data["layer_deps"].split(","):

                        layer_dep_obj = Layer_Version.objects.get(pk=layer_dep_id)
                        LayerVersionDependency.objects.get_or_create(layer_version=layer_version, depends_on=layer_dep_obj)
                        # Now add them to the project, we could get an execption
                        # if the project now contains the exact
                        # dependency already (like modified on another page)
                        try:
                            prj_layer, prj_layer_created = ProjectLayer.objects.get_or_create(layercommit=layer_dep_obj, project=prj)
                        except:
                            continue

                        if prj_layer_created:
                            layers_added.append({'id': layer_dep_obj.id, 'name': Layer.objects.get(id=layer_dep_obj.layer_id).name})


                # If an old layer version exists in our project then remove it
                for prj_layers in ProjectLayer.objects.filter(project=prj):
                    dup_layer_v = Layer_Version.objects.filter(id=prj_layers.layercommit_id, layer_id=layer.id)
                    if len(dup_layer_v) >0 :
                        prj_layers.delete()

                # finally add the imported layer (version id) to the project
                ProjectLayer.objects.create(layercommit=layer_version, project=prj,optional=1)

            else:
                # We didn't create a layer version so back out now and clean up.
                if layer_created:
                    layer.delete()

                return HttpResponse(jsonfilter({"error": "Uncaught error: Could not create layer version"}), content_type = "application/json")


        return HttpResponse(jsonfilter({"error": "ok", "imported_layer" : { "name" : layer.name, "id": layer_version.id },  "deps_added": layers_added }), content_type = "application/json")

    def xhr_updatelayer(request):

        def error_response(error):
            return HttpResponse(jsonfilter({"error": error}), content_type = "application/json")

        if not request.POST.has_key("layer_version_id"):
            return error_response("Please specify a layer version id")
        try:
            layer_version_id = request.POST["layer_version_id"]
            layer_version = Layer_Version.objects.get(id=layer_version_id)
        except:
            return error_response("Cannot find layer to update")


        if request.POST.has_key("vcs_url"):
            layer_version.layer.vcs_url = request.POST["vcs_url"]
        if request.POST.has_key("dirpath"):
            layer_version.dirpath = request.POST["dirpath"]
        if request.POST.has_key("commit"):
            layer_version.commit = request.POST["commit"]
        if request.POST.has_key("up_branch"):
            layer_version.up_branch_id = int(request.POST["up_branch"])

        if request.POST.has_key("add_dep"):
            lvd = LayerVersionDependency(layer_version=layer_version, depends_on_id=request.POST["add_dep"])
            lvd.save()

        if request.POST.has_key("rm_dep"):
            rm_dep = LayerVersionDependency.objects.get(layer_version=layer_version, depends_on_id=request.POST["rm_dep"])
            rm_dep.delete()

        if request.POST.has_key("summary"):
            layer_version.layer.summary = request.POST["summary"]
        if request.POST.has_key("description"):
            layer_version.layer.description = request.POST["description"]

        try:
            layer_version.layer.save()
            layer_version.save()
        except:
            return error_response("Could not update layer version entry")

        return HttpResponse(jsonfilter({"error": "ok",}), content_type = "application/json")



    def importlayer(request):
        template = "importlayer.html"
        context = {
        }
        return render(request, template, context)


    def layers(request):
        if not 'project_id' in request.session:
            raise Exception("invalid page: cannot show page without a project")

        template = "layers.html"
        # define here what parameters the view needs in the GET portion in order to
        # be able to display something.  'count' and 'page' are mandatory for all views
        # that use paginators.
        (pagesize, orderby) = _get_parameters_values(request, 100, 'layer__name:+')
        mandatory_parameters = { 'count': pagesize,  'page' : 1, 'orderby' : orderby };
        retval = _verify_parameters( request.GET, mandatory_parameters )
        if retval:
            return _redirect_parameters( 'layers', request.GET, mandatory_parameters)

        # boilerplate code that takes a request for an object type and returns a queryset
        # for that object type. copypasta for all needed table searches
        (filter_string, search_term, ordering_string) = _search_tuple(request, Layer_Version)

        prj = Project.objects.get(pk = request.session['project_id'])

        queryset_all = prj.compatible_layerversions()

        queryset_all = _get_queryset(Layer_Version, queryset_all, filter_string, search_term, ordering_string, '-layer__name')

        object_list = set([x.get_equivalents_wpriority(prj)[0] for x in queryset_all])
        object_list = list(object_list)


        # retrieve the objects that will be displayed in the table; layers a paginator and gets a page range to display
        layer_info = _build_page_range(Paginator(object_list, request.GET.get('count', 10)),request.GET.get('page', 1))


        context = {
            'projectlayerset' : jsonfilter(map(lambda x: x.layercommit.id, prj.projectlayer_set.all())),
            'objects' : layer_info,
            'objectname' : "layers",
            'default_orderby' : 'layer__name:+',

            'tablecols' : [
                {   'name': 'Layer',
                    'orderfield': _get_toggle_order(request, "layer__name"),
                    'ordericon' : _get_toggle_order_icon(request, "layer__name"),
                },
                {   'name': 'Description',
                    'dclass': 'span4',
                    'clclass': 'description',
                },
                {   'name': 'Git repository URL',
                    'dclass': 'span6',
                    'clclass': 'git-repo', 'hidden': 1,
                    'qhelp': "The Git repository for the layer source code",
                },
                {   'name': 'Subdirectory',
                    'clclass': 'git-subdir',
                    'hidden': 1,
                    'qhelp': "The layer directory within the Git repository",
                },
                {   'name': 'Revision',
                    'clclass': 'branch',
                    'qhelp': "The Git branch, tag or commit. For the layers from the OpenEmbedded layer source, the revision is always the branch compatible with the Yocto Project version you selected for this project",
                },
                {   'name': 'Dependencies',
                    'clclass': 'dependencies',
                    'qhelp': "Other layers a layer depends upon",
                },
                {   'name': 'Add | Delete',
                    'dclass': 'span2',
                    'qhelp': "Add or delete layers to / from your project ",
                },
            ]
        }

        response = render(request, template, context)
        _save_parameters_cookies(response, pagesize, orderby, request)

        return response

    def layerdetails(request, layerid):
        template = "layerdetails.html"
        limit = 10

        if request.GET.has_key("limit"):
            request.session['limit'] = request.GET['limit']

        if request.session.has_key('limit'):
            limit = request.session['limit']

        layer_version = Layer_Version.objects.get(pk = layerid)

        targets_query = Recipe.objects.filter(layer_version=layer_version)

        # Targets tab query functionality
        if request.GET.has_key('targets_search'):
            targets_query = targets_query.filter(
                Q(name__icontains=request.GET['targets_search']) |
                Q(summary__icontains=request.GET['targets_search']))

        targets = _build_page_range(Paginator(targets_query.order_by("name"), limit), request.GET.get('tpage', 1))

        machines_query = Machine.objects.filter(layer_version=layer_version)

        # Machines tab query functionality
        if request.GET.has_key('machines_search'):
            machines_query = machines_query.filter(
                Q(name__icontains=request.GET['machines_search']) |
                Q(description__icontains=request.GET['machines_search']))

        machines = _build_page_range(Paginator(machines_query.order_by("name"), limit), request.GET.get('mpage', 1))

        context = {
            'layerversion': layer_version,
            'layer_in_project' : ProjectLayer.objects.filter(project_id=request.session['project_id'],layercommit=layerid).count(),
            'machines': machines,
            'targets': targets,
            'total_targets': Recipe.objects.filter(layer_version=layer_version).count(),

            'total_machines': Machine.objects.filter(layer_version=layer_version).count(),
        }
        return render(request, template, context)

    def targets(request):
        if not 'project_id' in request.session:
            raise Exception("invalid page: cannot show page without a project")

        template = 'targets.html'
        (pagesize, orderby) = _get_parameters_values(request, 100, 'name:+')
        mandatory_parameters = { 'count': pagesize,  'page' : 1, 'orderby' : orderby }
        retval = _verify_parameters( request.GET, mandatory_parameters )
        if retval:
            return _redirect_parameters( 'all-targets', request.GET, mandatory_parameters)
        (filter_string, search_term, ordering_string) = _search_tuple(request, Recipe)

        prj = Project.objects.get(pk = request.session['project_id'])
        queryset_all = Recipe.objects.filter(Q(layer_version__up_branch__name= prj.release.name) | Q(layer_version__build__in = prj.build_set.all())).filter(name__regex=r'.{1,}.*')

        queryset_with_search = _get_queryset(Recipe, queryset_all, None, search_term, ordering_string, '-name')

        # get unique values for 'name', and select the maximum ID for each entry (the max id is the newest one)

        # force evaluation of the query here; to process the MAX/GROUP BY, a temporary table is used, on which indexing is very slow
        # by forcing the evaluation here we also prime the caches
        queryset_with_search_maxids = map(lambda i: i[0], list(queryset_with_search.values('name').distinct().annotate(max_id=Max('id')).values_list('max_id')))

        queryset_with_search = queryset_with_search.filter(id__in=queryset_with_search_maxids).select_related('layer_version', 'layer_version__layer', 'layer_version__up_branch', 'layer_source')


        # retrieve the objects that will be displayed in the table; targets a paginator and gets a page range to display
        target_info = _build_page_range(Paginator(queryset_with_search, request.GET.get('count', 10)),request.GET.get('page', 1))

        for e in target_info.object_list:
            e.preffered_layerversion = e.layer_version.get_equivalents_wpriority(prj)[0]
            e.vcs_link_url = Layer.objects.filter(name = e.preffered_layerversion.layer.name).exclude(vcs_web_file_base_url__isnull=True)[0].vcs_web_file_base_url
            if e.vcs_link_url != None:
                fp = e.preffered_layerversion.dirpath + "/" + e.file_path
                e.vcs_link_url = e.vcs_link_url.replace('%path%', fp)
                e.vcs_link_url = e.vcs_link_url.replace('%branch%', e.preffered_layerversion.up_branch.name)

        context = {
            'projectlayerset' : jsonfilter(map(lambda x: x.layercommit.id, prj.projectlayer_set.all().select_related("layercommit"))),
            'objects' : target_info,
            'objectname' : "recipes",
            'default_orderby' : 'name:+',

            'tablecols' : [
                {   'name': 'Recipe',
                    'orderfield': _get_toggle_order(request, "name"),
                    'ordericon' : _get_toggle_order_icon(request, "name"),
                },
                {   'name': 'Recipe version',
                    'dclass': 'span2',
                },
                {   'name': 'Description',
                    'dclass': 'span5',
                    'clclass': 'description',
                },
                {   'name': 'Recipe file',
                    'clclass': 'recipe-file',
                    'hidden': 1,
                    'dclass': 'span5',
                },
                {   'name': 'Section',
                    'clclass': 'target-section',
                    'hidden': 1,
                    'orderfield': _get_toggle_order(request, "section"),
                    'ordericon': _get_toggle_order_icon(request, "section"),
                    'orderkey': "section",
                },
                {   'name': 'License',
                    'clclass': 'license',
                    'hidden': 1,
                    'orderfield': _get_toggle_order(request, "license"),
                    'ordericon': _get_toggle_order_icon(request, "license"),
                    'orderkey': "license",
                },
                {   'name': 'Layer',
                    'clclass': 'layer',
                    'orderfield': _get_toggle_order(request, "layer_version__layer__name"),
                    'ordericon': _get_toggle_order_icon(request, "layer_version__layer__name"),
                    'orderkey': "layer_version__layer__name",
                },
                {   'name': 'Revision',
                    'clclass': 'branch',
                    'qhelp': "The Git branch, tag or commit. For the layers from the OpenEmbedded layer source, the revision is always the branch compatible with the Yocto Project version you selected for this project.",
                    'hidden': 1,
                },
            ]
        }

        if 'project_id' in request.session:
            context['tablecols'] += [
                {   'name': 'Build',
                    'dclass': 'span2',
                    'qhelp': "Add or delete targets to / from your project ",
                }, ]

        response = render(request, template, context)
        _save_parameters_cookies(response, pagesize, orderby, request)

        return response

    def machines(request):
        if not 'project_id' in request.session:
            raise Exception("invalid page: cannot show page without a project")

        template = "machines.html"
        # define here what parameters the view needs in the GET portion in order to
        # be able to display something.  'count' and 'page' are mandatory for all views
        # that use paginators.
        (pagesize, orderby) = _get_parameters_values(request, 100, 'name:+')
        mandatory_parameters = { 'count': pagesize,  'page' : 1, 'orderby' : orderby };
        retval = _verify_parameters( request.GET, mandatory_parameters )
        if retval:
            return _redirect_parameters( 'machines', request.GET, mandatory_parameters)

        # boilerplate code that takes a request for an object type and returns a queryset
        # for that object type. copypasta for all needed table searches
        (filter_string, search_term, ordering_string) = _search_tuple(request, Machine)

        prj = Project.objects.get(pk = request.session['project_id'])
        compatible_layers = prj.compatible_layerversions()

        queryset_all = Machine.objects.filter(layer_version__in=compatible_layers)
        queryset_all = _get_queryset(Machine, queryset_all, None, search_term, ordering_string, 'name')

        queryset_all = queryset_all.prefetch_related('layer_version')


        # Make sure we only show machines / layers which are compatible
        # with the current project

        project_layers = ProjectLayer.objects.filter(project_id=request.session['project_id']).values_list('layercommit',flat=True)

        # Now we need to weed out the layers which will appear as duplicated
        # because they're from a layer source which doesn't need to be used
        for machine in queryset_all:
            to_rm = machine.layer_version.get_equivalents_wpriority(prj)[1:]
            if len(to_rm) > 0:
               queryset_all = queryset_all.exclude(layer_version__in=to_rm)

        machine_info = _build_page_range(Paginator(queryset_all, request.GET.get('count', 100)),request.GET.get('page', 1))

        context = {
            'objects' : machine_info,
            'project_layers' : project_layers,
            'objectname' : "machines",
            'default_orderby' : 'name:+',

            'tablecols' : [
                {   'name': 'Machine',
                    'orderfield': _get_toggle_order(request, "name"),
                    'ordericon' : _get_toggle_order_icon(request, "name"),
                    'orderkey' : "name",
                },
                {   'name': 'Description',
                    'dclass': 'span5',
                    'clclass': 'description',
                },
                {   'name': 'Layer',
                    'clclass': 'layer',
                    'orderfield': _get_toggle_order(request, "layer_version__layer__name"),
                    'ordericon' : _get_toggle_order_icon(request, "layer_version__layer__name"),
                    'orderkey' : "layer_version__layer__name",
                },
                {   'name': 'Revision',
                    'clclass': 'branch',
                    'qhelp' : "The Git branch, tag or commit. For the layers from the OpenEmbedded layer source, the revision is always the branch compatible with the Yocto Project version you selected for this project",
                    'hidden': 1,
                },
                {   'name' : 'Machine file',
                    'clclass' : 'machinefile',
                    'hidden' : 1,
                },
                {   'name': 'Select',
                    'dclass': 'select span2',
                    'qhelp': "Sets the selected machine as the project machine. You can only have one machine per project",
                },

            ]
        }

        response = render(request, template, context)
        _save_parameters_cookies(response, pagesize, orderby, request)

        return response


    def get_project_configvars_context():
        # Vars managed outside of this view
        vars_managed = {
            'MACHINE', 'BBLAYERS'
        }

        vars_blacklist  = {
            'DL_DR','PARALLEL_MAKE','BB_NUMBER_THREADS','SSTATE_DIR',
            'BB_DISKMON_DIRS','BB_NUMBER_THREADS','CVS_PROXY_HOST','CVS_PROXY_PORT',
            'DL_DIR','PARALLEL_MAKE','SSTATE_DIR','SSTATE_DIR','SSTATE_MIRRORS','TMPDIR',
            'all_proxy','ftp_proxy','http_proxy ','https_proxy'
            }

        vars_fstypes  = {
            'btrfs','cpio','cpio.gz','cpio.lz4','cpio.lzma','cpio.xz','cramfs',
            'elf','ext2','ext2.bz2','ext2.gz','ext2.lzma', 'ext4', 'ext4.gz', 'ext3','ext3.gz','hddimg',
            'iso','jffs2','jffs2.sum','squashfs','squashfs-lzo','squashfs-xz','tar.bz2',
            'tar.lz4','tar.xz','tartar.gz','ubi','ubifs','vmdk'
        }

        return(vars_managed,sorted(vars_fstypes),vars_blacklist)

    def projectconf(request, pid):
        template = "projectconf.html"

        try:
            prj = Project.objects.get(id = pid)
        except Project.DoesNotExist:
            return HttpResponseNotFound("<h1>Project id " + pid + " is unavailable</h1>")

        # remove blacklist and externally managed varaibles from this list
        vars_managed,vars_fstypes,vars_blacklist = get_project_configvars_context()
        configvars = ProjectVariable.objects.filter(project_id = pid).all()
        for var in vars_managed:
            configvars = configvars.exclude(name = var)
        for var in vars_blacklist:
            configvars = configvars.exclude(name = var)

        context = {
            'configvars':       configvars,
            'vars_managed':     vars_managed,
            'vars_fstypes':     vars_fstypes,
            'vars_blacklist':   vars_blacklist,
        }

        try:
            context['distro'] =  ProjectVariable.objects.get(project = prj, name = "DISTRO").value
            context['distro_defined'] = "1"
        except ProjectVariable.DoesNotExist:
            pass
        try:
            context['fstypes'] =  ProjectVariable.objects.get(project = prj, name = "IMAGE_FSTYPES").value
            context['fstypes_defined'] = "1"
        except ProjectVariable.DoesNotExist:
            pass
        try:
            context['image_install_append'] =  ProjectVariable.objects.get(project = prj, name = "IMAGE_INSTALL_append").value
            context['image_install_append_defined'] = "1"
        except ProjectVariable.DoesNotExist:
            pass
        try:
            context['package_classes'] =  ProjectVariable.objects.get(project = prj, name = "PACKAGE_CLASSES").value
            context['package_classes_defined'] = "1"
        except ProjectVariable.DoesNotExist:
            pass
        try:
            context['sdk_machine'] =  ProjectVariable.objects.get(project = prj, name = "SDKMACHINE").value
            context['sdk_machine_defined'] = "1"
        except ProjectVariable.DoesNotExist:
            pass

        return render(request, template, context)

    def projectbuilds(request, pid):
        template = 'projectbuilds.html'
        buildrequests = BuildRequest.objects.filter(project_id = pid).exclude(state__lte = BuildRequest.REQ_INPROGRESS).exclude(state=BuildRequest.REQ_DELETED)

        try:
            context, pagesize, orderby = _build_list_helper(request, buildrequests, False)
        except InvalidRequestException as e:
            return _redirect_parameters(projectbuilds, request.GET, e.response, pid = pid)

        response = render(request, template, context)
        _save_parameters_cookies(response, pagesize, orderby, request)

        return response


    def _file_name_for_artifact(b, artifact_type, artifact_id):
        file_name = None
        # Target_Image_File file_name
        if artifact_type == "imagefile":
            file_name = Target_Image_File.objects.get(target__build = b, pk = artifact_id).file_name

        elif artifact_type == "buildartifact":
            file_name = BuildArtifact.objects.get(build = b, pk = artifact_id).file_name

        elif artifact_type ==  "licensemanifest":
            file_name = Target.objects.get(build = b, pk = artifact_id).license_manifest_path

        elif artifact_type == "tasklogfile":
            file_name = Task.objects.get(build = b, pk = artifact_id).logfile

        elif artifact_type == "logmessagefile":
            file_name = LogMessage.objects.get(build = b, pk = artifact_id).pathname
        else:
            raise Exception("FIXME: artifact type %s not implemented" % (artifact_type))

        return file_name


    def build_artifact(request, build_id, artifact_type, artifact_id):
        if artifact_type in ["cookerlog"]:
            # these artifacts are saved after building, so they are on the server itself
            def _mimetype_for_artifact(path):
                try:
                    import magic

                    # fair warning: this is a mess; there are multiple competing and incompatible
                    # magic modules floating around, so we try some of the most common combinations

                    try:    # we try ubuntu's python-magic 5.4
                        m = magic.open(magic.MAGIC_MIME_TYPE)
                        m.load()
                        return m.file(path)
                    except AttributeError:
                        pass

                    try:    # we try python-magic 0.4.6
                        m = magic.Magic(magic.MAGIC_MIME)
                        return m.from_file(path)
                    except AttributeError:
                        pass

                    try:    # we try pip filemagic 1.6
                        m = magic.Magic(flags=magic.MAGIC_MIME_TYPE)
                        return m.id_filename(path)
                    except AttributeError:
                        pass

                    return "binary/octet-stream"
                except ImportError:
                    return "binary/octet-stream"
            try:
                # match code with runbuilds.Command.archive()
                build_artifact_storage_dir = os.path.join(ToasterSetting.objects.get(name="ARTIFACTS_STORAGE_DIR").value, "%d" % int(build_id))
                file_name = os.path.join(build_artifact_storage_dir, "cooker_log.txt")

                fsock = open(file_name, "r")
                content_type=_mimetype_for_artifact(file_name)

                response = HttpResponse(fsock, content_type = content_type)

                response['Content-Disposition'] = 'attachment; filename=' + os.path.basename(file_name)
                return response
            except IOError:
                context = {
                    'build' : Build.objects.get(pk = build_id),
                }
                return render(request, "unavailable_artifact.html", context)

        else:
            # retrieve the artifact directly from the build environment
            return _get_be_artifact(request, build_id, artifact_type, artifact_id)


    def _get_be_artifact(request, build_id, artifact_type, artifact_id):
        try:
            b = Build.objects.get(pk=build_id)
            if b.buildrequest is None or b.buildrequest.environment is None:
                raise Exception("Artifact not available for download (missing build request or build environment)")

            file_name = _file_name_for_artifact(b, artifact_type, artifact_id)
            fsock = None
            content_type='application/force-download'

            if file_name is None:
                raise Exception("Could not handle artifact %s id %s" % (artifact_type, artifact_id))
            else:
                content_type = MimeTypeFinder.get_mimetype(file_name)
                fsock = b.buildrequest.environment.get_artifact(file_name)
                file_name = os.path.basename(file_name) # we assume that the build environment system has the same path conventions as host

            response = HttpResponse(fsock, content_type = content_type)

            # returns a file from the environment
            response['Content-Disposition'] = 'attachment; filename=' + file_name
            return response
        except IOError:
            context = {
                'build' : Build.objects.get(pk = build_id),
            }
            return render(request, "unavailable_artifact.html", context)

    # This returns the mru object that is needed for the
    # managed_mrb_section.html template
    def _managed_get_latest_builds():
        build_mru = BuildRequest.objects.all()
        build_mru = list(build_mru.filter(Q(state__lt=BuildRequest.REQ_COMPLETED) or Q(state=BuildRequest.REQ_DELETED)).order_by("-pk")) + list(build_mru.filter(state__in=[BuildRequest.REQ_COMPLETED, BuildRequest.REQ_FAILED]).order_by("-pk")[:3])
        return build_mru


    def projects(request):
        template="projects.html"

        (pagesize, orderby) = _get_parameters_values(request, 10, 'updated:-')
        mandatory_parameters = { 'count': pagesize,  'page' : 1, 'orderby' : orderby }
        retval = _verify_parameters( request.GET, mandatory_parameters )
        if retval:
            return _redirect_parameters( 'all-projects', request.GET, mandatory_parameters)

        queryset_all = Project.objects.all()

        # boilerplate code that takes a request for an object type and returns a queryset
        # for that object type. copypasta for all needed table searches
        (filter_string, search_term, ordering_string) = _search_tuple(request, Project)
        queryset_with_search = _get_queryset(Project, queryset_all, None, search_term, ordering_string, '-updated')
        queryset = _get_queryset(Project, queryset_all, filter_string, search_term, ordering_string, '-updated')

        # retrieve the objects that will be displayed in the table; projects a paginator and gets a page range to display
        project_info = _build_page_range(Paginator(queryset, pagesize), request.GET.get('page', 1))

        # build view-specific information; this is rendered specifically in the builds page, at the top of the page (i.e. Recent builds)
        build_mru = _managed_get_latest_builds()

        # translate the project's build target strings
        fstypes_map = {};
        for project in project_info:
            try:
                targets = Target.objects.filter( build_id = project.get_last_build_id() )
                comma = "";
                extensions = "";
                for t in targets:
                    if ( not t.is_image ):
                        continue
                    tif = Target_Image_File.objects.filter( target_id = t.id )
                    for i in tif:
                        s=re.sub('.*tar.bz2', 'tar.bz2', i.file_name)
                        if s == i.file_name:
                            s=re.sub('.*\.', '', i.file_name)
                        if None == re.search(s,extensions):
                            extensions += comma + s
                            comma = ", "
                fstypes_map[project.id]=extensions
            except (Target.DoesNotExist,IndexError):
                fstypes_map[project.id]=project.get_last_imgfiles

        context = {
                'mru' : build_mru,

                'objects' : project_info,
                'objectname' : "projects",
                'default_orderby' : 'id:-',
                'search_term' : search_term,
                'total_count' : queryset_with_search.count(),
                'fstypes' : fstypes_map,
                'build_FAILED' : Build.FAILED,
                'build_SUCCEEDED' : Build.SUCCEEDED,
                'tablecols': [
                    {'name': 'Project',
                    'orderfield': _get_toggle_order(request, "name"),
                    'ordericon':_get_toggle_order_icon(request, "name"),
                    'orderkey' : 'name',
                    },
                    {'name': 'Last activity on',
                    'clclass': 'updated',
                    'qhelp': "Shows the starting date and time of the last project build. If the project has no builds, it shows the date the project was created",
                    'orderfield': _get_toggle_order(request, "updated", True),
                    'ordericon':_get_toggle_order_icon(request, "updated"),
                    'orderkey' : 'updated',
                    },
                    {'name': 'Release',
                    'qhelp' : "The version of the build system used by the project",
                    'orderfield': _get_toggle_order(request, "release__name"),
                    'ordericon':_get_toggle_order_icon(request, "release__name"),
                    'orderkey' : 'release__name',
                    },
                    {'name': 'Machine',
                    'qhelp': "The hardware currently selected for the project",
                    },
                    {'name': 'Number of builds',
                    'qhelp': "How many builds have been run for the project",
                    },
                    {'name': 'Last build outcome', 'clclass': 'loutcome',
                    'qhelp': "Tells you if the last project build completed successfully or failed",
                    },
                    {'name': 'Recipe', 'clclass': 'ltarget',
                    'qhelp': "The last recipe that was built in this project",
                    },
                    {'name': 'Errors', 'clclass': 'lerrors',
                    'qhelp': "How many errors were encountered during the last project build (if any)",
                    },
                    {'name': 'Warnings', 'clclass': 'lwarnings',
                    'qhelp': "How many warnigns were encountered during the last project build (if any)",
                    },
                    {'name': 'Image files', 'clclass': 'limagefiles', 'hidden': 1,
                    'qhelp': "The root file system types produced by the last project build",
                    },
                    ]
            }
        return render(request, template, context)

    def buildrequestdetails(request, pid, brid):
        template = "buildrequestdetails.html"
        context = {
            'buildrequest' : BuildRequest.objects.get(pk = brid, project_id = pid)
        }
        return render(request, template, context)


else:
    # these are pages that are NOT available in interactive mode
    def managedcontextprocessor(request):
        return {
            "projects": [],
            "MANAGED" : toastermain.settings.MANAGED,
            "DEBUG" : toastermain.settings.DEBUG,
            "TOASTER_BRANCH": toastermain.settings.TOASTER_BRANCH,
            "TOASTER_REVISION" : toastermain.settings.TOASTER_REVISION,
        }


    # shows the "all builds" page for interactive mode; this is the old code, simply moved
    def builds(request):
        template = 'build.html'
        # define here what parameters the view needs in the GET portion in order to
        # be able to display something.  'count' and 'page' are mandatory for all views
        # that use paginators.
        (pagesize, orderby) = _get_parameters_values(request, 10, 'completed_on:-')
        mandatory_parameters = { 'count': pagesize,  'page' : 1, 'orderby' : orderby }
        retval = _verify_parameters( request.GET, mandatory_parameters )
        if retval:
            return _redirect_parameters( 'all-builds', request.GET, mandatory_parameters)

        # boilerplate code that takes a request for an object type and returns a queryset
        # for that object type. copypasta for all needed table searches
        (filter_string, search_term, ordering_string) = _search_tuple(request, Build)
        # post-process any date range filters
        filter_string,daterange_selected = _modify_date_range_filter(filter_string)
        queryset_all = Build.objects.exclude(outcome = Build.IN_PROGRESS)
        queryset_with_search = _get_queryset(Build, queryset_all, None, search_term, ordering_string, '-completed_on')
        queryset = _get_queryset(Build, queryset_all, filter_string, search_term, ordering_string, '-completed_on')

        # retrieve the objects that will be displayed in the table; builds a paginator and gets a page range to display
        build_info = _build_page_range(Paginator(queryset, pagesize), request.GET.get('page', 1))

        # build view-specific information; this is rendered specifically in the builds page, at the top of the page (i.e. Recent builds)
        build_mru = Build.objects.order_by("-started_on")[:3]

        # calculate the exact begining of local today and yesterday, append context
        context_date,today_begin,yesterday_begin = _add_daterange_context(queryset_all, request, {'started_on','completed_on'})

        # set up list of fstypes for each build
        fstypes_map = {};
        for build in build_info:
            targets = Target.objects.filter( build_id = build.id )
            comma = "";
            extensions = "";
            for t in targets:
                if ( not t.is_image ):
                    continue
                tif = Target_Image_File.objects.filter( target_id = t.id )
                for i in tif:
                    s=re.sub('.*tar.bz2', 'tar.bz2', i.file_name)
                    if s == i.file_name:
                        s=re.sub('.*\.', '', i.file_name)
                    if None == re.search(s,extensions):
                        extensions += comma + s
                        comma = ", "
            fstypes_map[build.id]=extensions

        # send the data to the template
        context = {
                # specific info for
                    'mru' : build_mru,
                # TODO: common objects for all table views, adapt as needed
                    'objects' : build_info,
                    'objectname' : "builds",
                    'default_orderby' : 'completed_on:-',
                    'fstypes' : fstypes_map,
                    'search_term' : search_term,
                    'total_count' : queryset_with_search.count(),
                    'daterange_selected' : daterange_selected,
                # Specifies the display of columns for the table, appearance in "Edit columns" box, toggling default show/hide, and specifying filters for columns
                    'tablecols' : [
                    {'name': 'Outcome',                                                # column with a single filter
                     'qhelp' : "The outcome tells you if a build successfully completed or failed",     # the help button content
                     'dclass' : "span2",                                                # indication about column width; comes from the design
                     'orderfield': _get_toggle_order(request, "outcome"),               # adds ordering by the field value; default ascending unless clicked from ascending into descending
                     'ordericon':_get_toggle_order_icon(request, "outcome"),
                      # filter field will set a filter on that column with the specs in the filter description
                      # the class field in the filter has no relation with clclass; the control different aspects of the UI
                      # still, it is recommended for the values to be identical for easy tracking in the generated HTML
                     'filter' : {'class' : 'outcome',
                                 'label': 'Show:',
                                 'options' : [
                                             ('Successful builds', 'outcome:' + str(Build.SUCCEEDED), queryset_with_search.filter(outcome=str(Build.SUCCEEDED)).count()),  # this is the field search expression
                                             ('Failed builds', 'outcome:'+ str(Build.FAILED), queryset_with_search.filter(outcome=str(Build.FAILED)).count()),
                                             ]
                                }
                    },
                    {'name': 'Recipe',                                                 # default column, disabled box, with just the name in the list
                     'qhelp': "What you built (i.e. one or more recipes or image recipes)",
                     'orderfield': _get_toggle_order(request, "target__target"),
                     'ordericon':_get_toggle_order_icon(request, "target__target"),
                    },
                    {'name': 'Machine',
                     'qhelp': "The machine is the hardware for which you are building a recipe or image recipe",
                     'orderfield': _get_toggle_order(request, "machine"),
                     'ordericon':_get_toggle_order_icon(request, "machine"),
                     'dclass': 'span3'
                    },                           # a slightly wider column
                    {'name': 'Started on', 'clclass': 'started_on', 'hidden' : 1,      # this is an unchecked box, which hides the column
                     'qhelp': "The date and time you started the build",
                     'orderfield': _get_toggle_order(request, "started_on", True),
                     'ordericon':_get_toggle_order_icon(request, "started_on"),
                     'filter' : {'class' : 'started_on',
                                 'label': 'Show:',
                                 'options' : [
                                             ("Today's builds" , 'started_on__gte:'+today_begin.strftime("%Y-%m-%d"), queryset_all.filter(started_on__gte=today_begin).count()),
                                             ("Yesterday's builds",
                                                 'started_on__gte!started_on__lt:'
                                                     +yesterday_begin.strftime("%Y-%m-%d")+'!'
                                                     +today_begin.strftime("%Y-%m-%d"),
                                                 queryset_all.filter(
                                                     started_on__gte=yesterday_begin,
                                                     started_on__lt=today_begin
                                                     ).count()),
                                             ("Build date range", 'daterange', 1, '', 'started_on'),
                                             ]
                                }
                     },
                    {'name': 'Completed on',
                     'qhelp': "The date and time the build finished",
                     'orderfield': _get_toggle_order(request, "completed_on", True),
                     'ordericon':_get_toggle_order_icon(request, "completed_on"),
                     'orderkey' : 'completed_on',
                     'filter' : {'class' : 'completed_on',
                                 'label': 'Show:',
                                 'options' : [
                                             ("Today's builds" , 'completed_on__gte:'+today_begin.strftime("%Y-%m-%d"), queryset_all.filter(completed_on__gte=today_begin).count()),
                                             ("Yesterday's builds",
                                                 'completed_on__gte!completed_on__lt:'
                                                     +yesterday_begin.strftime("%Y-%m-%d")+'!'
                                                     +today_begin.strftime("%Y-%m-%d"),
                                                 queryset_all.filter(
                                                     completed_on__gte=yesterday_begin,
                                                     completed_on__lt=today_begin
                                                     ).count()),
                                             ("Build date range", 'daterange', 1, '', 'completed_on'),
                                             ]
                                }
                    },
                    {'name': 'Failed tasks', 'clclass': 'failed_tasks',                # specifing a clclass will enable the checkbox
                     'qhelp': "How many tasks failed during the build",
                     'filter' : {'class' : 'failed_tasks',
                                 'label': 'Show:',
                                 'options' : [
                                             ('Builds with failed tasks', 'task_build__outcome:4', queryset_with_search.filter(task_build__outcome=4).count()),
                                             ('Builds without failed tasks', 'task_build__outcome:NOT4', queryset_with_search.filter(~Q(task_build__outcome=4)).count()),
                                             ]
                                }
                    },
                    {'name': 'Errors', 'clclass': 'errors_no',
                     'qhelp': "How many errors were encountered during the build (if any)",
                     'orderfield': _get_toggle_order(request, "errors_no", True),
                     'ordericon':_get_toggle_order_icon(request, "errors_no"),
                     'orderkey' : 'errors_no',
                     'filter' : {'class' : 'errors_no',
                                 'label': 'Show:',
                                 'options' : [
                                             ('Builds with errors', 'errors_no__gte:1', queryset_with_search.filter(errors_no__gte=1).count()),
                                             ('Builds without errors', 'errors_no:0', queryset_with_search.filter(errors_no=0).count()),
                                             ]
                                }
                    },
                    {'name': 'Warnings', 'clclass': 'warnings_no',
                     'qhelp': "How many warnings were encountered during the build (if any)",
                     'orderfield': _get_toggle_order(request, "warnings_no", True),
                     'ordericon':_get_toggle_order_icon(request, "warnings_no"),
                     'orderkey' : 'warnings_no',
                     'filter' : {'class' : 'warnings_no',
                                 'label': 'Show:',
                                 'options' : [
                                             ('Builds with warnings','warnings_no__gte:1', queryset_with_search.filter(warnings_no__gte=1).count()),
                                             ('Builds without warnings','warnings_no:0', queryset_with_search.filter(warnings_no=0).count()),
                                             ]
                                }
                    },
                    {'name': 'Log',
                     'dclass': "span4",
                     'qhelp': "Path to the build main log file",
                     'clclass': 'log', 'hidden': 1,
                     'orderfield': _get_toggle_order(request, "cooker_log_path"),
                     'ordericon':_get_toggle_order_icon(request, "cooker_log_path"),
                     'orderkey' : 'cooker_log_path',
                    },
                    {'name': 'Time', 'clclass': 'time', 'hidden' : 1,
                     'qhelp': "How long it took the build to finish",
                     'orderfield': _get_toggle_order(request, "timespent", True),
                     'ordericon':_get_toggle_order_icon(request, "timespent"),
                     'orderkey' : 'timespent',
                    },
                    {'name': 'Image files', 'clclass': 'output',
                     'qhelp': "The root file system types produced by the build. You can find them in your <code>/build/tmp/deploy/images/</code> directory",
                        # TODO: compute image fstypes from Target_Image_File
                    },
                    ]
                }

        # merge daterange values
        context.update(context_date)

        response = render(request, template, context)
        _save_parameters_cookies(response, pagesize, orderby, request)
        return response




    def newproject(request):
        return render(request, 'landing_not_managed.html')

    def project(request, pid):
        return render(request, 'landing_not_managed.html')

    def xhr_projectbuild(request, pid):
        return render(request, 'landing_not_managed.html')

    def xhr_build(request):
        return render(request, 'landing_not_managed.html')

    def xhr_projectinfo(request):
        return render(request, 'landing_not_managed.html')

    def xhr_projectedit(request, pid):
        return render(request, 'landing_not_managed.html')

    def xhr_datatypeahead(request):
        return render(request, 'landing_not_managed.html')

    def xhr_configvaredit(request, pid):
        return render(request, 'landing_not_managed.html')

    def importlayer(request):
        return render(request, 'landing_not_managed.html')

    def layers(request):
        return render(request, 'landing_not_managed.html')

    def layerdetails(request, layerid):
        return render(request, 'landing_not_managed.html')

    def targets(request):
        return render(request, 'landing_not_managed.html')

    def machines(request):
        return render(request, 'landing_not_managed.html')

    def projectconf(request, pid):
        return render(request, 'landing_not_managed.html')

    def projectbuilds(request, pid):
        return render(request, 'landing_not_managed.html')

    def build_artifact(request, build_id, artifact_type, artifact_id):
        return render(request, 'landing_not_managed.html')

    def projects(request):
        return render(request, 'landing_not_managed.html')

    def xhr_importlayer(request):
        return render(request, 'landing_not_managed.html')

    def xhr_updatelayer(request):
        return render(request, 'landing_not_managed.html')

    def buildrequestdetails(request, pid, brid):
        return render(request, 'landing_not_managed.html')
