# openstackchef.bbclass
# Copyright (c) 2014 Wind River Systems, Inc.
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to
# deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
# 
#
# This class provides a framework for openstack services like python-neutron
# or python-nova to register their configuration files so that they can be
# recreated at run-time by chef-solo. Inheriting 
# this class involves exposing configuration files from which this class
# creates chef-solo templates. These template files are later used by chef-solo 
# to  recreate the configuration files.
#
# For the templates files to be created, the recipes are expected 
# to define the following variables variables:
# 
# SRCNAME: 
# This is the name of the package, neutron for example for the package
# python-neutron. It's appended to the names of template files and also
# used in the creation of package based temporary files. A default value 
# of ${BPN} is assigned to this variable when it's not defined by recipes
# inheriting class.
# 
# CHEF_SERVICES_CONF_FILES
#
# This variable provides the list of configuration files 
# exposed to this class by the recipes inheriting the class. 
# These files are expected to be in the image( ${D}) directory, though ${D} 
# directory is excluded from the file name. Definition of this variable
# by recipe files is optional.
# eg. 
# CHEF_SERVICES_CONF_FILES="\
#   ${sysconfdir}/chef/neutron/plugins/linuxbridge/linuxbridge_conf.ini.rb \
#   ${sysconfdir}/chef/neutron/neutron.conf
#   "
#
#INITSCRIPT_PACKAGES
#This variable provides a mechanism for recipes inheriting this class
#to provide a list of services to start/stop when their configuration files
#are recreated on disk.
#This variable is assumed to be provided by all recipes that register a set
#of configuration files with this class. Failing to do so will lead to
#service not reloading the newly created configuration file(s) at run-time.
#
# 
#INITSCRIPT_NAME_x or INITSCRIPT_NAME
#This variable is also assumed to be set by recipes inheriting this class. 
#It specifies the names of the services to start/stop as specified above. 
#Like the variable immediately above, failure to provide this variable will
#lead to mis-configuration of the service at run-time.
#
#
#INITSCRIPT_PARAMS_x or INITSCRIPT_PARAMS
#Like the last two variable above, this variable is also assumed to be set
#by recipes inheriting this class. It is used to extract the run-level
#priority of the INITSCRIPT_NAME variable(s). Unlike, the previous two 
#variables, a default run-level is assigned to the script when this variable
#defaults to the string 'default' 
#
#
# CHEF_SERVICES_SPECIAL_FUNC
# This variable is optional, and is the name of a shell callback function. 
# Unlike the placeholder/value substitution which this class does,
# there are times when recipes need to do more than a simple placeholder/
# value substitution. This is made possible with the use of the callback
# function.
# The callback function should be defined by the recipe(s) inheriting
# this class. When this variable is defined, this class will call the 
# callback function and pass it the name of the file to manipulate
# in the form of the variable CHEF_SERVICES_FILE_NAME. This variable 
# is then accessed in the callback function in the recipe file as
# ${CHEF_SERVICES_FILE_NAME}
#
inherit hosts openstackchef_inc

#Call this function after the do_install function have executed in 
#recipes inheriting this class, this ensures that we get configuration
#files that have been moved to the images directory
addtask deploychef_install before do_package after do_install
python do_deploychef_install() { 
    if deploychef_not_rootfs_class(d) and \
        deploychef_openstackchef_enabled(d):
        #copy configuration files from packages inheriting class
        template_files_tuple = deploychef_copy_conf_files(d)
        #convert configuration files into templates for chefsolo
        deploychef_make_templates( d, template_files_tuple)
        #Generate a list of startup/shutdown services
        deploychef_make_startup_shutdown_list(d)
}

#Use of ROOTFS_POSTPROCESS_COMMAND enables us to hook into the post 
#rootfs creation process and influence the work of openstack_configure_hosts.
#However, to ensure that our function deploychef_rootfs_postprocess_commands 
#is called after openstack_configure_hosts and not before it,
#we add it in front of our callback function here.
ROOTFS_POSTPROCESS_COMMAND += "openstack_configure_hosts ; deploychef_rootfs_postprocess_commands ; "

