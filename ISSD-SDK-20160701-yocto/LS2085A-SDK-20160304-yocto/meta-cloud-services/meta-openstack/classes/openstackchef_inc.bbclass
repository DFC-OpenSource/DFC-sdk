# openstackchef_inc.bbclass
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
# FITNESS FOR A PARTICULAR PURPOSE AND NONE INFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
#
#This class is a helper class for openstackchef.bbclass and should not be be
#inherited on it's own. It implements the functionalities used in 
#openstackchef.bbclass
CHEFPN = "openstackchef"
#This variable is here to support legacy use of recipes that inherit this
#class. By default these recipes will be built without openstackchef support.
#Set this variable to 'yes' in your local.conf or similar to enable
#support for OPENSTACKCHEF.
OPENSTACKCHEF_ENABLED ?=''
#This variable is defined in most of the openstack services, however,
#it defaults to bare package name for packages that do not define it.
SRCNAME ?= "${BPN}"
#This is the base directory of where deploychef's templates files
#reside in the classes that inherit it.
CHEF_TEMPLATE_BASE="${D}${sysconfdir}/${CHEFPN}"
CHEF_PACKAGE_BASE="${WORKDIR}/package${sysconfdir}/${CHEFPN}"
CHEF_IMAGE_BASE="${D}${sysconfdir}"
CHEF_ROOTFS_BASE="${sysconfdir}/${CHEFPN}"
#These are the prefixs and suffixs to create chefsolo-like placeholders
ERB_PREFIX = "<%=node[:"
ERB_SUFFIX = "]%>"
#These prefix and suffix are used to create our default values
ERB_DEFAULT_PREFIX="default["
ERB_DEFAULT_SUFFIX="]"
#Chefsolo template file extension
TEMPLATE_EXTENSION='.erb'
#Build deploychef package since this class has run-time and buil-time dependency 
#on it
DEPENDS_${PN} += "${@bb.utils.contains('DISTRO_FEATURES', 'chef', 'deploychef', '', d)}"
RDEPENDS_${PN} += "${@bb.utils.contains('DISTRO_FEATURES', 'chef', 'deploychef', '', d)}"
CHEF_VERSION = '1'
#For classes that define a special substitution function, this variable is 
#set by this class and specifies the file named passed back to these function
#for any special substitution.
CHEF_SERVICES_FILE_NAME ?=''

#These are hard coded in the recipes files where they are used. 
ADMIN_TENANT_NAME ?= 'admin'
DEMO_USER ?= 'demo'
#This is dafault value used by update-rc.d script at runtime when
#build-time update-rd.d class specifies INITSCRIPT_PARAMS as 'defaults' 
CHEF_INITSCRIPT_PARAMS='defaults 20 10'

def deploychef_services_default_sub_dict(d):
    """Provides our placeholder/value substitution dictionary for global use
    
    This function returns as dictionary containing the default substitution pattern
    as follows:
    :<key>: Name of placeholder variable
    :<value>: A tuple consisting  of the place holder variable followed 
    by its default value
    """
    sub_dict={}
    #neutron
    sub_dict['SERVICE_TENANT_NAME'] = ('%SERVICE_TENANT_NAME%' ,
    '${SERVICE_TENANT_NAME}') 
    sub_dict['SERVICE_PASSWORD'] = ('%SERVICE_PASSWORD%' , '${SERVICE_PASSWORD}')

    sub_dict['DB_USER'] = ('%DB_USER%', '${DB_USER}')
    sub_dict['DB_PASSWORD'] = ('%DB_PASSWORD%' , '${DB_PASSWORD}')
    sub_dict['CONTROLLER_IP'] = ('%CONTROLLER_IP%', '${CONTROLLER_IP}')
    sub_dict['CONTROLLER_HOST'] = ('%CONTROLLER_HOST%', '${CONTROLLER_HOST}')
    #nova
    sub_dict['COMPUTE_IP'] = ('%COMPUTE_IP%', '${COMPUTE_IP}')
    sub_dict['COMPUTE_HOST'] = ('%COMPUTE_HOST%', '${COMPUTE_HOST}')
    sub_dict['LIBVIRT_IMAGES_TYPE'] = ('%LIBVIRT_IMAGES_TYPE%' , '${LIBVIRT_IMAGES_TYPE}')
    sub_dict['OS_PASSWORD'] = ('%OS_PASSWORD%' , '${ADMIN_PASSWORD}')
    sub_dict['SERVICE_TOKEN'] = ('%SERVICE_TOKEN%' , '${SERVICE_TOKEN}')
    #swfit
    sub_dict['ADMIN_TENANT_NAME'] = ('%ADMIN_TENANT_NAME%' , '${ADMIN_TENANT_NAME}')
    sub_dict['ADMIN_USER'] = ('%ADMIN_USER%' , '${ADMIN_TENANT_NAME}')
    sub_dict['ADMIN_PASSWORD'] = ('%ADMIN_PASSWORD%' , '${ADMIN_PASSWORD}')
    sub_dict['DEMO_USER'] = ('%DEMO_USER%' , '${DEMO_USER}')
    sub_dict['DEMO_PASSWORD'] = ('%DEMO_PASSWORD%' , '${ADMIN_PASSWORD}')
    #ceilometer
    sub_dict['CEILOMETER_SECRET'] = ('%CEILOMETER_SECRET%' , '${CEILOMETER_SECRET}')

    #keystone
    sub_dict['TOKEN_FORMAT'] = ('%TOKEN_FORMAT%' , '${TOKEN_FORMAT}')
    sub_dict['METADATA_SHARED_SECRET'] = ('%METADATA_SHARED_SECRET%' , '${METADATA_SHARED_SECRET}')
    #cinder
    sub_dict['CINDER_BACKUP_BACKEND_DRIVER'] = ('%CINDER_BACKUP_BACKEND_DRIVER%' , '${CINDER_BACKUP_BACKEND_DRIVER}')
    #cloud-init
    sub_dict['MANAGE_HOSTS'] = ('%MANAGE_HOSTS%' , '${MANAGE_HOSTS}')
    sub_dict['HOSTNAME'] = ('%HOSTNAME%' , '${MY_HOST}')
    #postgresql
    sub_dict['DB_DATADIR'] = ('%DB_DATADIR%' , '${DB_DATADIR}')
    #glance
    sub_dict['GLANCE_DEFAULT_STORE'] = ('%GLANCE_DEFAULT_STORE%' , '${GLANCE_DEFAULT_STORE}')
    
    #barbican
    sub_dict['BARBICAN_MAX_PACKET_SIZE'] = ('%BARBICAN_MAX_PACKET_SIZE%' , '${BARBICAN_MAX_PACKET_SIZE}')
    #ceph
    sub_dict['PUBLIC_IP'] = ('%PUBLIC_IP%' , '${CONTROLLER_IP}')
    sub_dict['PRIVATE_IP'] = ('%PRIVATE_IP%' , '${MY_IP}')
    sub_dict['PUBLIC_DOMAIN'] = ('%PUBLIC_DOMAIN%' , '${PUBLIC_DOMAIN}')
    sub_dict['HOST_NAME'] = ('%HOST_NAME%' , '${MY_HOST}')
    sub_dict['CEPH_BACKING_FILE_SIZE'] = ('%CEPH_BACKING_FILE_SIZE%' , '${CEPH_BACKING_FILE_SIZE}')

    #Most of the services have SERVICE_USER define but the values are different
    service_user = d.getVar('SRCNAME', True)
    if service_user:
        service_user = service_user.upper() + '_SERVICE_USER'
        sub_dict[service_user] = ('%SERVICE_USER%' , '${SRCNAME}')
    return sub_dict

#This variable is the default sets of substitutions common to most of the
#services in an openstack installation. It means this package is not completely
#agnostic but at the same time it gives us the added advantage of not repeating 
#ourselves in the recipe/class files that inherits this class.
CHEF_SERVICES_DEFAULT_CONF_SUBS := "${@deploychef_services_default_sub_dict(d)}"


def deploychef_not_rootfs_class(d):
    """Filter out rootfs related classes

    This function is used to help us selectively executes the creation of template 
    files and daemon start/stop list. It allows us to executes certain functions 
    when the recipe inheriting this class is related to rootfs creation.
    """
    pkg = d.getVar('PN', True) or d.getVar('BPN', True)
    #Skip test if recipe/class calling this class is related to rootfs image
    #creation.
    if 'image-' in pkg:
        return False
    else:
        return True

def deploychef_openstackchef_enabled(d):
    """Find out if openstackchef class usage is enabled

    The variable OPENSTACKCHEF_ENABLED is use to support legacy functionality
    by recipes inheriting this class. Assinging it an empty string disables 
    the functionality of this class for that recipe file. 
    This function helps us detect when this variable is set to an empty string.
    """
    chef = d.getVar('OPENSTACKCHEF_ENABLED', True) 
    #Skip test if recipe/class calling this class is related to rootfs image
    #creation.
    if chef != '' :
        return True
    else:
        return False

def deploychef_make_startup_shutdown_list(d):
    """Create list of services to start/stop and save them to file

    This function uses the update-rc.d environment variables defined
    in the recipes/classes inheriting this class to create the 
    startup and shutdown services list as defined in the file. 
    This is important because, when we are replacing the configuration 
    files for the services, the services needs to be shutdown and 
    restarted after their configuration files are edited.
    Therefore, the following variables must be defined by classes inheriting 
    this class.

    INITSCRIPT_PACKAGES: A list of init scripts
    INITSCRIPT_PARAMS_x: The default start/stop priority for the above scripts
    """

    import os
    if d.getVar('INITSCRIPT_PACKAGES') or d.getVar('INITSCRIPT_NAME'):
        #script_list = bb.data.getVar('INITSCRIPT_PACKAGES', d , 1)
        script_list = d.getVar('INITSCRIPT_PACKAGES', True) or \ 
         d.getVar('INITSCRIPT_NAME', True)  #Some package do not define INITSCRIPT_PACKAGES
        msg="list of start/stop services: %s" % str(script_list)
        bb.debug(2, msg)
        base_dir = bb.data.getVar('CHEF_TEMPLATE_BASE', d, 1 )
        if not os.path.exists(base_dir):
            os.mkdir(base_dir)
        startup_file = os.path.join(base_dir, d.getVar('SRCNAME', True) +'-startup-list')
        shutdown_file = os.path.join(base_dir, d.getVar('SRCNAME', True) +'-shutdown-list')
        msg ="Startup and shutdown files %s %s saved to %s" % \
        ( startup_file, shutdown_file, base_dir)
        bb.note(msg)
        try:
            hStartup = open(startup_file, 'w')
            hShutdown = open(shutdown_file, 'w')
            for script_name in script_list.split():
                #Retrieve the default params for update-rc.d for this script
                #INITSCRIPT_PARAMS_nova-api="defaults 30 10"
                init_script_name ="INITSCRIPT_NAME_%s" % script_name
                init_script_name = d.getVar(init_script_name, True) or \
                d.getVar('INITSCRIPT_NAME', True)

                script_params ="INITSCRIPT_PARAMS_%s" % script_name
                script_params = d.getVar(script_params, True) or \
                d.getVar('INITSCRIPT_PARAMS', True)
                #If only defaults is provided as parameter, then use our default  priority
                if not script_params  or len(script_params.split()) < 3: 
                    script_params = d.getVar('CHEF_INITSCRIPT_PARAMS', True)
                if init_script_name: 
                    #eg. start 20 5 3 2 . stop 80 0 1 6 .
                    startup_priority = shutdown_priority = ''
                    if script_params.find('stop') > 0:
                        start, stop = script_params.split('stop')
                        start = start.split()
                        stop = stop.split()
                        startup_priority = start[1]
                        shutdown_priority = stop[0]
                    elif script_params.find('stop') == 0:
                        start, stop = script_params.split('start')
                        start = start.split()
                        stop = stop.split()
                        startup_priority = start[0]
                        shutdown_priority = stop[1]
                    else:
                        #"defaults 20 10" 
                        defaults = script_params.split()
                        startup_priority = defaults[1]
                        shutdown_priority = defaults[2]
                    #Arrange the script's startup/shutdown format as in rc.x
                    startup_string = "%s%s%s%s" %  ('S', startup_priority, init_script_name, os.linesep)
                    shutdown_string = "%s%s%s%s" % ('K', shutdown_priority, init_script_name, os.linesep)
                    hStartup.write(startup_string)
                    hShutdown.write(shutdown_string)
                    msg = "%s %s registered for startup and shutdown in" % \
                    (startup_string, shutdown_string)
                    bb.debug(2 , msg)
                else:
                    msg = "The variables INITSCRIPT_PARAMS_%s or INITSCRIPT_PARAMS \ 
                    are not set in %s, startup/shutdown list not created for %s: %s" \
                    % (script_name, d.getVar('FILE', True), d.getVar('SRCNAME', True), script_params)
                    raise bb.build.FuncFailed(msg)
            hStartup.close()
            hShutdown.close()
        except IOError, e:
            bb.error("Error opening startup/shutdown files %s %s,  %s %s" % \
            (startup_file, shutdown_file, d.getVar('FILE'), e))
    else:
        msg = "The variable INITSCRIPT_PACKAGES is not set in %s,  \
        startup/shutdown script will not be made for %s package" % \
        (d.getVar('FILE', True), d.getVar('SRCNAME', True))
        bb.build.FuncFailed(msg)

def deploychef_make_substitutions(d, sub_dict, attr_filename, sed_filename):
    """Make placeholder/value substitution in a file
    
    This function makes placeholder substitution into the file named 
    sed_filename and appends the default value for those substitution into
    the file named attr_filename.
    The substitution is based on sets of placeholder/value pair in the 
    dictionary called sub_dict.
    :sub_dict: name, placeholder and value dictionary
    :attr_filename: chef-solo attributes file name
    :sed_filename: configuration or template file
    """
    if attr_filename and sed_filename:
        if type(sub_dict) is dict:
            import os
            import re
            sHandle = open(sed_filename, 'r+')
            lines_in_conf_file = sHandle.read()
            #We only want to append to the list of defines needed by this class
            hFile= open(attr_filename,'a+')
            lines_in_file= hFile.read()
            for key in sub_dict.keys():
                placeholder , replacement = sub_dict[key]
                #Filter by placeholder so that our attribute file only include
                #defines for class being built 
                if re.search(placeholder,lines_in_conf_file):
                    #Make the substitution to create a template file out of
                    #the configuration file or just replace placeholder in
                    #configuration file
                    if d.getVar('TEMPLATE_EXTENSION', True) in sed_filename:
                        #Format the default attributes as expected by chefsolo
                        #for template file.
                        attr_string = d.getVar('ERB_DEFAULT_PREFIX', True)
                        attr_string += r'"' + key
                        attr_string += r'"' + d.getVar('ERB_DEFAULT_SUFFIX', True)
                        attr_string +=' = ' + r'"' + replacement + r'"'
                        #Only write default values that do not yet exist in file
                        #if key not in lines_in_file:
                        if not re.search(key,lines_in_file):
                            hFile.write("%s%s" % (attr_string, os.linesep))
                        #Replace the placeholders in the current file, with
                        #new_replacement
                        new_replacement = d.getVar('ERB_PREFIX') + key
                        new_replacement += d.getVar('ERB_SUFFIX')
                        lines_in_conf_file = re.sub(placeholder, new_replacement, lines_in_conf_file)
            #Write template file to disk
            sHandle.seek(0)
            sHandle.truncate()
            sHandle.write(lines_in_conf_file)
            sHandle.close()
            
            hFile.close()
            if False:
                msg = "Cannot read/write to attributes file %s as expected:%s"\
                % (attr_filename, bb.data.getVar('FILE', d))
                bb.build.FuncFailed(msg)
        else:
            msg = "The substitution dictionary variable sub_dict is not set  %s as expected "\
            % bb.data.getVar('FILE')
            bb.error(msg)
    else:
        msg = "Null file names passsed to function %s %s "\
        % (bb.data.getVar('FUNC',d), bb.data.getVar('FILE', d))
        bb.error(msg)



def deploychef_make_templates( d, conf_tuple=tuple()):
    """Create a template file out of a configuration file

    Using substitution placeholders and values in the substitution
    dictionary declared as CHEF_SERVICES_DEFAULT_CONF_SUBS, this function
    makes the substitution for all placeholders. If the file is a ruby template file, 
    it replaces the placeholders with a chefsolo expression; 
    thereby creating a chefsolo template file.
    
    :conf_tuple: List of configuration files
    """

    if len(conf_tuple):
        import os, ast
        base_dir = bb.data.getVar('CHEF_TEMPLATE_BASE', d, 1 )
        attr_file = os.path.join(base_dir, d.getVar('SRCNAME', True) + '-attributes.rb')
        msg ="Default attributes saved to %s" % attr_file
        if os.path.exists(attr_file):
            os.remove(attr_file)
        bb.note(msg)
        try:
            for file_name in conf_tuple:
                #If a special substitution function is defined for class 
                #inheriting this class, set the file name expected by special
                #function before calling the function 
                special_func_name = d.getVar('CHEF_SERVICES_SPECIAL_FUNC')
                if special_func_name:
                    bb.data.setVar('CHEF_SERVICES_FILE_NAME', file_name,\
                    d)
                    bb.build.exec_func(special_func_name, d)
                
                #Make the necessary susbstitutions using the default
                #substitutiin dictionary
                sub_dict = bb.data.getVar('CHEF_SERVICES_DEFAULT_CONF_SUBS', d , 1)
                msg = "The variable %s is not set in %s as a dictionary as expected "\
                % ('CHEF_SERVICES_DEFAULT_CONF_SUBS', bb.data.getVar('FILE', d))
                if sub_dict:
                    #Safely retrieve our python data structure
                    sub_dict = ast.literal_eval(sub_dict)
                    if type(sub_dict) is dict:
                        deploychef_make_substitutions(d, sub_dict, attr_file, file_name)
                    else:
                        raise bb.build.FuncFailed(msg)
                else:
                    raise bb.build.FuncFailed(msg)
                #Make the necessary susbstitutions using auxilliary dictionary
                #if provided by inheriting class
                sub_dict = bb.data.getVar('CHEF_SERVICES_CONF_SUBS', d , 1)
                if sub_dict:
                    sub_dict = ast.literal_eval(sub_dict)
                    msg = "The variable %s is not set in %s as a dictionary as expected "\
                    % ('CHEF_SERVICES_CONF_SUB', bb.data.getVar('FILE', d))
                    if type(sub_dict) is dict:
                        pass
                        deploychef_make_substitutions(d, sub_dict, attr_file, file_name)
                    else:
                        bb.build.FuncFailed(msg)
        except IOError, e:
            bb.error("Could not write to attribute file %s: in %s,  %s" % \
            (attr_file, d.getVar('FILE'), e))

def deploychef_copy_single_conf_file(d, dst_base, conf_file):
    """Create a chef-solo template from an openstack configuration file

    This function copies a single configuration file (conf_file)
    to destination directory (dst_dir) and return a tuple that contains
    both the absolute path of the conf_file and the template files it was
    copied as.
    """
    if conf_file: 
        import shutil
        import os
        #Create the absolute path to configuration file since it's with relative
        #to image directory
        image_base = d.getVar('D', True)
        if conf_file.startswith(os.sep):
            conf_file=conf_file[1:]
        abs_conf_path = os.path.join(image_base, conf_file)
        
        if os.path.exists(abs_conf_path):
            dst_base = os.path.join(dst_base, os.path.dirname(conf_file))
            #make room for the template file about to be created
            if not os.path.exists(dst_base):
                os.makedirs(dst_base)

            abs_template_file = os.path.basename(conf_file) + \
            d.getVar('TEMPLATE_EXTENSION', True) + '.' + d.getVar('SRCNAME', True)
            abs_template_file = os.path.join(dst_base, abs_template_file)
            #Copy conf file as template file
            shutil.copy(abs_conf_path, abs_template_file)
            msg = "\nConf file: %s\n Copied to: %s \n"\
            % (abs_conf_path, abs_template_file)
            bb.debug(2, msg)
            return (abs_conf_path, abs_template_file)
        else:
            msg = "Configuration file: %s in %s does not \
            exist" % (abs_conf_path, d.getVar('FILE'))
            raise bb.build.FuncFailed(msg)
    else:
        msg = "The specified configuration file destined for %s in %s is an empty string\n" \
        % (dst_base, d.getVar('FILE'))
        raise bb.build.FuncFailed(msg)


    
def deploychef_copy_conf_files(d): 
    """Copy openstack services' configuration files to be used as chef-solo templates

    Copy the configuration file(s) for the services under 
    ${D}${sysconfdir}/${CHEFPN}/<conf_file>. 
    The file(s) is/are assumed to be located in the images directory; ${D}<conf_file>
    And evaluate all necessary substitution in the configuration file.
    """
    abs_template_list = list()
    abs_conf_list = list()

    #Retrieve our string of configuration files
    conf_files = d.getVar('CHEF_SERVICES_CONF_FILES', True )
    #The template files that will be made from the configuration files will be
    #copied with reference to this base directory.
    dst_base = d.getVar('CHEF_TEMPLATE_BASE', True ) 
    if conf_files and len(conf_files.strip()):
        conf_files = conf_files.split()
        if len(conf_files) != 1:
            for conf_file in conf_files:
                abs_conf_path, abs_template_path = deploychef_copy_single_conf_file(d, \
                dst_base, conf_file)
                if abs_template_path:
                    #Save the absolute path to the template file 
                    abs_template_list.append(abs_template_path)
                if abs_conf_path:
                    #Save the absolute path to the configuration file
                    abs_conf_list.append(abs_conf_path)
        else:
            abs_conf_path, abs_template_path = deploychef_copy_single_conf_file(d,\
            dst_base, conf_files[0])
            if abs_template_path:
                #Save the absolute path to the template file 
                abs_template_list.append(abs_template_path)
            if abs_conf_path:
                #Save the absolute path to the template file 
                abs_conf_list.append(abs_conf_path)
        #Since the recipes no longer do the substitution in the
        #configuration files, let us do it for the configuration files
        deploychef_make_templates(d, tuple(abs_conf_list))
    else:
        msg = "The variable CHEF_SERVICES_CONF_FILES is not set"
        msg += " in %s as a list of files as expected" % d.getVar('FILE', True) 
        #raise bb.build.FuncFailed(msg)
        #No longer a requirement that all recipes inheriting this 
        #class must have a set of configuration files.
        bb.debug(2,msg)
    return tuple(abs_template_list)

def deploychef_postinst_substitutions(d, sub_dict, postinst):
    """Make value substitution in openstack services' postinstall scripts

    This function makes all necessary substitution in the 'setup' related postinsts 
    functions pgk_postinst_${PN}-setup. The substitution is base on entries in a 
    dictionary sub_dict. In addition it also updates the list of defined constansts 
    based on the values specified in dictionary or as specified by the recipe's 
    callback function.
    
    :sub_dict: name, placeholder and value substitution dictionary
    :postinst: content of an openstack service's postinstall script

    """
    if postinst:
        if type(sub_dict) is dict:
            import re
            base_dir = d.getVar('CHEF_PACKAGE_BASE', True)
            attr_filename = os.path.join(base_dir, d.getVar('SRCNAME', True) + '-attributes.rb')
            if os.path.exists(attr_filename):
                hFile= open(attr_filename,'a+')
                lines_in_file= hFile.read()
                for key in sub_dict.keys():
                    placeholder , replacement = sub_dict[key]
                    if replacement and ( re.search(placeholder, postinst) or \
                        re.search(replacement, postinst)):
                        #If there is any remaining placeholder in the current string
                        #replace it.
                        new_replacement = d.getVar('ERB_PREFIX') + key
                        new_replacement += d.getVar('ERB_SUFFIX')
                        
                        updated_postinst = re.sub(placeholder, new_replacement, postinst)
                        #If the placeholder has been substituted, look for the
                        #substitution and replace it with our template value
                        updated_postinst = re.sub(replacement, new_replacement, updated_postinst)
                        #Update our attributes file with the updated replacement
                        #string
                        attr_string = d.getVar('ERB_DEFAULT_PREFIX', True)
                        attr_string += r'"' + key
                        attr_string += r'"' + d.getVar('ERB_DEFAULT_SUFFIX', True)
                        attr_string +=' = ' + r'"' + replacement + r'"'
                        #Only write default values that do not yet exist in file
                        #if key not in lines_in_file:
                        if not re.search(key,lines_in_file):
                            hFile.write("%s%s" % (attr_string, os.linesep))

                        postinst_msg= "placeholder %s \n replacement %s \n updated_postinst :\n  %s \n" % \
                        (placeholder, replacement, updated_postinst)
                        bb.debug(2, postinst_msg)
                        postinst = updated_postinst
                hFile.close()
        else:
            msg = "The substitution dictionary variable sub_dict is not set  %s as expected "\
            % bb.data.getVar('FILE')
            bb.build.FuncFailed(msg)
    else:
        msg = "Null string passsed to function %s %s "\
        % (bb.data.getVar('FUNC',d), bb.data.getVar('FILE', d))
        bb.build.FuncFailed(msg)
    return postinst

def deploychef_update_package_postinsts(d):
    """Make placeholder/value substitution in openstack postinstall scripts

    This function searches all the 'setup' related post-install scripts for 
    references to placeholders of interest; such as %CONTROLLER_IP%.
    It replaces any such reference when it does find one with a placeholder
    (<%=node[:CONTROLLER_IP]%>); that essentially converts the post-install 
    script to a chefsolo template.
    """
    def update_postinst_package(pkg):
        bb.debug(1, 'Updating placeholders in postinst for pkg_postinst_%s scripts' % pkg)
        
        ldata = bb.data.createCopy(d)
        overrides = ldata.getVar("OVERRIDES", True)
        
        msg = "%s The override variable is %s" % (pkg, overrides)
        bb.note(msg)
        ldata.setVar("OVERRIDES", "%s:%s" % (pkg, overrides))
        
        bb.data.update_data(ldata)
        postinst = ldata.getVar('pkg_postinst', True)
        if postinst:
            #Make the necessary substitutions using the default
            #substitution dictionary
            overrides = d.getVar("OVERRIDES", True)
            msg = "%s The override variable is %s :\n %s " % (pkg, overrides, postinst)
            bb.note(msg)
            sub_dict = bb.data.getVar('CHEF_SERVICES_DEFAULT_CONF_SUBS', d , 1)
            msg = "The variable %s is not set in %s as a dictionary as expected "\
            % ('CHEF_SERVICES_DEFAULT_CONF_SUBS', bb.data.getVar('FILE', d))
            if sub_dict:
                import ast
                #Safely retrieve our python data structure
                sub_dict = ast.literal_eval(sub_dict)
                if type(sub_dict) is dict:
                    import re
                    updated_postinst = deploychef_postinst_substitutions(d, sub_dict, postinst)
                    #Replace the placeholders in postinst script if any
                    d.setVar('pkg_postinst_%s' % pkg, updated_postinst)
                else:
                    raise bb.build.FuncFailed(msg)
            else:
                raise bb.build.FuncFailed(msg)
        else:
            msg= "pkg_postinst_%s does not exist for %s\n" % (pkg, str(ldata))
            bb.note(msg)
            bb.build.FuncFailed(msg)
        
    packages = (d.getVar('PACKAGES', True) or "").split()
    if packages != []:
        for  pkg in packages:
            if pkg.endswith('setup'):
                update_postinst_package(pkg)
    
python populate_packages_append() {

    deploychef_update_package_postinsts(d)
}

def deploychef_add_file_to_FILES_PN(d, conf_file=None):
    """Add all directories under a file name to FILES_${PN} variable

    This function appends the name of the template file to the FILES_${PN}/${BPN}
    bitbake variable to avoid QA warning about files built but not 
    added to rootfs. $CHEF_TEMPLATE_BASE/conf_file. Note that conf_file
    is relative to the root filesystem as in /etc/neutron/neutron.conf
    The template file will be located in /etc/${CHEFPN}/etc/neutron/neutron.conf.rb
    Thefore, we need to make sure that all directories above the
    template file are added to FILES_${PN} variable.
    
    :conf_file: a chef-solo template file
    """
    import re
    import os
    #Perform an override so that we can update FILES_${PN} variables
    ldata = bb.data.createCopy(d)
    overrides = ldata.getVar("OVERRIDES", True)
    pkg = d.getVar('PN', True) or d.getVar('BPN', True)
    files = d.getVar('FILES_%s' % pkg, True)
    pkg_files = "FILES_%s" %  pkg    
    ldata.setVar("OVERRIDES", "%s:%s" % (pkg_files, overrides))
    bb.data.update_data(ldata)

    dest_base = d.getVar('CHEF_TEMPLATE_BASE')
    pkg_imagedir = d.getVar('CHEF_ROOTFS_BASE', True)
    #Add the packages image base directory if it does not already exist
    if re.search(pkg_imagedir, files) == None:
        #All the directory and all files in it
        files = "%s %s" % ( files, pkg_imagedir)
        files = "%s %s%s*" % ( files, pkg_imagedir, os.sep )
        d.setVar('FILES_%s' % pkg, files)
        msg= "Updated FILES_%s: %s for base images dir" % (pkg, d.getVar('FILES_%s' % pkg, files))
        bb.debug(2,msg)
    #All the files and all sub directory leading up to the package image base directory
    if conf_file:
        rel_basedir = os.path.dirname(conf_file)
        if rel_basedir.startswith(os.sep):
            rel_basedir = rel_basedir[1:]
        rel_basedir = os.path.join(pkg_imagedir, rel_basedir)
        if re.search(rel_basedir, files) == None:
            files = "%s %s" % ( files, rel_basedir)
            files = "%s %s%s*" % ( files, rel_basedir, os.sep )
            while rel_basedir.count(os.sep) > 4:
                #Must be above /etc/chef/etc/ 
                rel_basedir_list = rel_basedir.split(os.sep)
                rel_basedir = os.sep.join(rel_basedir_list[:-1])
                if re.search(rel_basedir, files) == None:
                    #All the directory and files in it
                    files = "%s %s" % ( files, rel_basedir)
                    files = "%s  %s%s*" % ( files, rel_basedir, os.sep )
                    bb.note(files)
            bb.debug(2, files)
            d.setVar('FILES_%s' % pkg, files)
            msg= "Updated FILES_%s: %s " % (pkg, d.getVar('FILES_%s' % pkg, files))
            bb.debug(2,msg)

def deploychef_update_FILES_PN_variable(d):
    """Indicate that the created chef-solo templates should be packaged
    
    This function ensures that all the templates files which are based off
    of configuration files exposed to this class are packaged up when they
    are copied from the images directory to the various packages folders
    This avoids the QA warning such as: 
    WARNING: For recipe python-neutron, the following files/directories were installed 
    but not shipped in any package:
    """
    conf_files = d.getVar('CHEF_SERVICES_CONF_FILES', True )
    if conf_files and len(conf_files.strip()):
        import shutil
        import os
        for conf_file in conf_files.split():
            deploychef_add_file_to_FILES_PN(d, conf_file)
    else:
        #Add the directory containing the start/stop scripts
        deploychef_add_file_to_FILES_PN(d)


python populate_packages_prepend() {

    deploychef_update_FILES_PN_variable(d)
}

#The sets of functions below are for post rootfs processing. Preparing files
#for chefsolo is a two stage process. First we must create the required files
#in the package's image directory; and this is mostly done by the functions
#above. 
#And then we aggregate the files from their respective package directories
#and put them together for the deploychef package in the expected
#location.
CHEF_ROOT_DIR="${IMAGE_ROOTFS}/${sysconfdir}/${CHEFPN}"
CHEF_CONF_DIR="${CHEF_ROOT_DIR}/${sysconfdir}"
INITD_DIR="${IMAGE_ROOTFS}/${sysconfdir}/init.d"
POSTINSTS_DIR="${IMAGE_ROOTFS}/${sysconfdir}/rpm-postinsts"
DEPLOYCHEF_DIR="${IMAGE_ROOTFS}/opt/deploychef"
DEPLOYCHEF_TEMPLATES_DIR="${DEPLOYCHEF_DIR}/cookbooks/openstack/templates/default"
ATTRIBUTES_DIR="${DEPLOYCHEF_DIR}/cookbooks/openstack/attributes"
ATTRIBUTES_FILE="${ATTRIBUTES_DIR}/default.rb"

deploychef_copy_host_files() {
   #The /etc/hosts & /etc/hostname files are written during the rootfs
   #post process, therefore the only way of making templates out of them 
   #is to hook into the rootfs post process command.
    if [ -f "${IMAGE_ROOTFS}/${sysconfdir}/hosts" ]; then
        #Convert etc/hosts to chefsolo template
        cp ${IMAGE_ROOTFS}/${sysconfdir}/hosts ${IMAGE_ROOTFS}/${sysconfdir}/${CHEFPN}/${sysconfdir}/hosts.erb
        sed -e "s,${CONTROLLER_IP},${ERB_PREFIX}CONTROLLER_IP${ERB_SUFFIX},g" -i \
        ${IMAGE_ROOTFS}/${sysconfdir}/${CHEFPN}/${sysconfdir}/hosts.erb
        sed -e "s,${CONTROLLER_HOST},${ERB_PREFIX}CONTROLLER_HOST${ERB_SUFFIX},g" -i \
        ${IMAGE_ROOTFS}/${sysconfdir}/${CHEFPN}/${sysconfdir}/hosts.erb
        
        sed -e "s,${COMPUTE_IP},${ERB_PREFIX}COMPUTE_IP${ERB_SUFFIX},g" -i \
        ${IMAGE_ROOTFS}/${sysconfdir}/${CHEFPN}/${sysconfdir}/hosts.erb
        sed -e "s,${COMPUTE_HOST},${ERB_PREFIX}COMPUTE_HOST${ERB_SUFFIX},g" -i \
        ${IMAGE_ROOTFS}/${sysconfdir}/${CHEFPN}/${sysconfdir}/hosts.erb
        #Create an attribute file for /etc/hosts 
        attr_string="${ERB_DEFAULT_PREFIX}\"COMPUTE_IP\"${ERB_DEFAULT_SUFFIX} = \"${COMPUTE_IP}\""
        echo "$attr_string" > ${IMAGE_ROOTFS}/${sysconfdir}/${CHEFPN}/hosts-attributes.rb
        attr_string="${ERB_DEFAULT_PREFIX}\"COMPUTE_HOST\"${ERB_DEFAULT_SUFFIX} = \"${COMPUTE_HOST}\""
        echo "$attr_string" >> ${IMAGE_ROOTFS}/${sysconfdir}/${CHEFPN}/hosts-attributes.rb
        attr_string="${ERB_DEFAULT_PREFIX}\"CONTROLLER_IP\"${ERB_DEFAULT_SUFFIX} = \"${CONTROLLER_IP}\""
        echo "$attr_string" >> ${IMAGE_ROOTFS}/${sysconfdir}/${CHEFPN}/hosts-attributes.rb
        attr_string="${ERB_DEFAULT_PREFIX}\"CONTROLLER_HOST\"${ERB_DEFAULT_SUFFIX} = \"${CONTROLLER_HOST}\""
        echo "$attr_string" >> ${IMAGE_ROOTFS}/${sysconfdir}/${CHEFPN}/hosts-attributes.rb
    fi

    if [ -f "${IMAGE_ROOTFS}/${sysconfdir}/hostname" ]; then
        #Convert etc/hostname to chefsolo template
        cp ${IMAGE_ROOTFS}/${sysconfdir}/hostname ${IMAGE_ROOTFS}/${sysconfdir}/${CHEFPN}/${sysconfdir}/hostname.erb
        sed -e "s,${MY_HOST},${ERB_PREFIX}HOSTNAME${ERB_SUFFIX},g" -i \
        ${IMAGE_ROOTFS}/${sysconfdir}/${CHEFPN}/${sysconfdir}/hostname.erb
        #Create an attribute file for /etc/hostname 
        attr_string="${ERB_DEFAULT_PREFIX}\"HOSTNAME\"${ERB_DEFAULT_SUFFIX} = \"${MY_HOST}\""
        echo "$attr_string" > ${IMAGE_ROOTFS}/${sysconfdir}/${CHEFPN}/hostname-attributes.rb
    fi
}


combine_services_daemons(){
    if [ -n $1 ]; then
        file_suffix=$1
        rm -f ${DEPLOYCHEF_DIR}/$file_suffix
        #combine the list of shutdown/startup scripts
        find "${CHEF_ROOT_DIR}/" -name "*$file_suffix" 2> /dev/null | while read fname; do 
            service_cont=$(cat $fname)
            for line in $service_cont; do 
                service=$(echo $line | awk -F"[SK][0-9]+" '{print $2}')
                if [ -e ${INITD_DIR}/$service ]; then
                    echo $line >> ${DEPLOYCHEF_DIR}/$file_suffix
                fi
            done
        done
    fi
}

#This function combines the attributes of all the sevices into
#a default.rb attributes file.
combine_services_attributes(){
    file_suffix='attributes.rb'
    mkdir -p ${ATTRIBUTES_DIR}; rm -f ${ATTRIBUTES_FILE} 2>/dev/null
    #combine the list of shutdown/starup scripts
    find "${CHEF_ROOT_DIR}/" -name "*$file_suffix" 2> /dev/null | \
    while read fname; do
        cat $fname | while read line_in_file; do
        index=$(echo  $line_in_file | awk -F'"' '{print $2}')
        #Only append attributes that are not in the default.rb attributes file
        if [ ! `grep -l $index ${ATTRIBUTES_FILE}` ]; then
            echo $line_in_file >> ${ATTRIBUTES_FILE}
        fi
    done
    done
}

#This function copies the templates to deploychef directory from
#within the packages directories
copy_templates_in_place(){
    #copy rpm-postinsts and config templates into templates directory
    mkdir -p ${DEPLOYCHEF_TEMPLATES_DIR}
    #First copy all our configuration template files
    if [ -d ${CHEF_CONF_DIR} ]; then
        cp -rf ${CHEF_CONF_DIR} ${DEPLOYCHEF_TEMPLATES_DIR}
    fi
    #Now copy the rpm-postinsts files into cookbooks/templates/default/etc/
    if [ -d ${POSTINSTS_DIR} ]; then
        cp -rf ${POSTINSTS_DIR} ${DEPLOYCHEF_TEMPLATES_DIR}/${sysconfdir}
        #Move the files to base of the templates directory, where chef-solo 
        #expects them
        cp -f ${POSTINSTS_DIR}/* ${DEPLOYCHEF_TEMPLATES_DIR}/.
    fi
    
}

filter_node_dependent_templates(){
    if [ -d ${DEPLOYCHEF_TEMPLATES_DIR} ]; then
        find "${DEPLOYCHEF_TEMPLATES_DIR}/" -name "*.erb*" 2> /dev/null | \
        while read fname; do
            config_file=$(echo  $fname | awk -F'/default' '{print $2}' | awk \
                -F'.erb' '{print $1}')
            #If the base configuration file does not exist on this node
            #remove it.
            if [ ! -f ${IMAGE_ROOTFS}$config_file ]; then
                rm -f "$fname"
            else
                #Move the file to the default template directory where
                #chefsolo expect them 
                cp "$fname"  "${DEPLOYCHEF_TEMPLATES_DIR}"
            fi
        done
    fi
}

#This function is our post rootfs hook, it enables
#us to do what we wish to do during rootfs creation process.
deploychef_rootfs_postprocess_commands() {
   
    if [ -n "${OPENSTACKCHEF_ENABLED}" ]; then
        deploychef_copy_host_files
        combine_services_daemons 'shutdown-list'
        combine_services_daemons 'startup-list'
        combine_services_attributes
        copy_templates_in_place
        filter_node_dependent_templates
    else
        #Let us delete the deploychef init script that runs
        #chef-solo at boot-up from rootfs
        rm -f ${INITD_DIR}/deploychef 2> /dev/null
    fi
    #We nolonger have need for /etc/${CHEFPN} directory on rootfs
    #Not even at run-time
    rm -rf "${CHEF_ROOT_DIR}" 
}

