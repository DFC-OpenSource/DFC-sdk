# default.rb
#
# Copyright (c) 2014 Wind River Systems, Inc.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
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
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
# This is our main recipe that chef-solo bakes to configure our openstack deployment
require 'find'
domain = node['hostname']
domain += " " + node['platform']
domain += " " + node['platform_version']
domain += " " + node['ipaddress']
log domain  do
        level:info
end

#The definition of IMAGES_ROOTS is to facilitate switching between development
#environment and run-time environment
node.default["IMAGE_ROOTFS"] = ""
#node.default["IMAGE_ROOTFS"] = "/opt/rootfs/TestDeploychef"
node.default["DEPLOYCHEF_ROOT_DIR"] = "#{node[:IMAGE_ROOTFS]}/opt/deploychef"
node.default["DEPLOYCHEF_TEMPLATE_DIR"] = "#{node[:DEPLOYCHEF_ROOT_DIR]}/cookbooks/openstack/templates/default"

node.default["ETC_DIR"]= "#{node[:IMAGE_ROOTFS]}/etc"
node.default["INITD_DIR"]= "#{node[:ETC_DIR]}/init.d"
node.default["POSTINSTS_DIR"]= "#{node[:ETC_DIR]}/rpm-postinsts"
node.default["CONTROLLER_DAEMON"]= "#{node[:INITD_DIR]}/nova-api"
node.default["COMPUTE_DAEMON"]= "#{node[:INITD_DIR]}/nova-compute"

#Indicate whether or not we successfully created the configuration files
#from templates.
$chefsolo_success = false
#This function enables us to over-ride the hostname and ip address
#attributes based on the type of node installation
#we are running on.
def update_ip_and_hostname()
    if  File.executable?(node[:CONTROLLER_DAEMON]) and \
        File.executable?(node[:COMPUTE_DAEMON])
        #All in one installation
        if node[:ipaddress].length
            node.default["CONTROLLER_IP"]=node.default["COMPUTE_IP"]=node.default["PUBLIC_IP"]=node[:ipaddress]
        end
        if node[:hostname].length
            node.default["CONTROLLER_HOST"]=node.default["COMPUTE_HOST"]=node[:hostname]
        end
        node.default["NODE_TYPE"] ="allinone"
    elsif File.executable?(node[:CONTROLLER_DAEMON])
        if node[:ipaddress].length
            node.default["CONTROLLER_IP"]=node.default["PUBLIC_IP"]=node[:ipaddress]
        end
        if node[:hostname].length
            node.default["CONTROLLER_HOST"]=node[:hostname]
        end
        node.default["NODE_TYPE"] ="controller"
    else
        if node[:ipaddress].length
            node.default["COMPUTE_IP"]=node.default["PUBLIC_IP"]=node[:ipaddress]
        end
        if node[:hostname].length
            node.default["COMPUTE_HOST"]=node[:hostname]
        end
        node.default["NODE_TYPE"] ="compute"
    end
    #Both private and public IP's default to an empty string in ceph
    #So provide default values when this is the case
    if not #{default[:PRIVATE_IP]}.length
        node.default["PRIVATE_IP"]="127.0.0.1"
    end
    node.default["PUBIC_DOMAIN"]="#{node[:PUBLIC_IP]}/24"
end

def make_config_files_from_templates()
    #Make it easier to move from development environment to target
    output_dir = node[:ETC_DIR]
    template_dir = node[:DEPLOYCHEF_TEMPLATE_DIR] + '/etc'
    #See if output directory exist if not create one
    if not File.directory?(output_dir)
        execute "Create #{output_dir} #{template_dir} directory" do
            command "mkdir -p #{output_dir}"
        end
    end
    #Get the list of all template files and create their corresponding config
    #files
    dirs = Dir.glob(template_dir)
    for dir in dirs  do
        next if File.file?(dir) or dir == '.' or dir == '..'
        Find.find(dir) do | file_name |
            if File.file?(file_name)
                abs_path_conf_file = "#{file_name}"
                if  abs_path_conf_file.include?(".erb.")
                    abs_path_conf_file, throw_away = abs_path_conf_file.split(".erb")
                else
                    abs_path_conf_file.gsub!(".erb","")
                end
                base_path, abs_path_conf_file = abs_path_conf_file.split('/default')
                base_path = node[:IMAGE_ROOTFS] + File.dirname(abs_path_conf_file)
                abs_path_conf_file = "#{base_path}/" + File.basename(abs_path_conf_file) 
                #This test is only true for test bed but for sake of portability 
                #we leave it in place
                if not File.exist?(base_path) 
                    execute "Creating conf file dir: #{abs_path_conf_file}" do
                        command "mkdir -p #{base_path}"
                    end
                elsif File.exist?(abs_path_conf_file)
                    execute "Delete file to force recreation: #{abs_path_conf_file}" do
                        command "rm -f #{abs_path_conf_file}"
                    end
                end
                execute "Created file: #{base_path}: #{abs_path_conf_file}" do
                    command "echo #{abs_path_conf_file}"
                end
                if File.executable?(file_name)
                    template abs_path_conf_file do
                        source File.basename(file_name)
                        #Preserve mode, owner and group
                        mode "0755"
                    end
                else
                    template abs_path_conf_file do
                        source File.basename(file_name)
                        #Preserve mode, owner and group
                        mode "0644"
                    end
                end
                $chefsolo_success = true
            end
        end
    end
    if $chefsolo_success 
        execute "Make the postinsts script executables" do
            command "touch #{node[:ETC_DIR]}/chefsolo.ran"
        end
    end
end

#Generate scripts and cofiguration files
update_ip_and_hostname
make_config_files_from_templates

=begin
service  host[:hostname] do
    action :restart
endf
=end
