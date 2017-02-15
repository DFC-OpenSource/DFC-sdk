#!/usr/bin/env python

import sys
import grp
import pwd
import os
import libvirt
import ConfigParser
import subprocess
import shutil
import distutils.spawn
import platform

# this does a very basic test to see if the required packages
# are installed, extend list as required
def checkPackages():
        sys_ok = True
        check_apps = [ "virsh", "qemu-system-x86_64", "libvirtd" ]
        for app in check_apps:
                if distutils.spawn.find_executable(app) == None:
                        print( "Missing: " + app)
                        sys_ok = False
        if not sys_ok:
                print("The required libvirt/qemu packages are missing...")
                distro = platform.dist()[0]
                if distro == "debian" or distro == "Ubuntu":
                        print( "This appears to be a Debian/Ubuntu distribution\nPlease install " +
                                "packages like libvirt-bin, qemu-system-x86,..." )
                elif distro == "redhat" or distro == "fedora":
                        print( "This appears to be a Redhat/Fedora distribution\nPlease install " +
                                "packages like libvirt-client, libvirt-daemon, qemu-system-x86, ..." )
                exit(1)
        return

def networkInterfaces():
        ifaces = []
        for line in open('/proc/net/dev', 'r'):
                if_info = line.split(":", 1)
                if len(if_info) > 1:
                        ifaces.append( if_info[0].strip() )
        return ifaces

def destroyNetwork(conn, network_name):
        networks = conn.listNetworks() + conn.listDefinedNetworks()
        if network_name in networks:
                try:
                        nw = conn.networkLookupByName(network_name)
                        if nw.isActive():
                                nw.destroy()
                        nw.undefine()
                except:
                        print( "Failed to destroy network: %s" % network_name )
                        exit( 1 )

def restartDomain(conn, domain_name):
        try:
                domain = conn.lookupByName(domain_name)
        except:
                print( "restartDomain: Warning domain " + domain_name + " doesn't exist." )
                return
        if domain.isActive():
                domain.reboot()

def destroyDomain(conn, auto_destroy, domain_name):
        try:
                domain = conn.lookupByName(domain_name)
        except:
                return
        if domain.isActive():
                if auto_destroy:
                        print( "Auto destroy enabled, destroying old instance of domain %s" % domain_name )
                        domain.destroy()
                else:
                        print( "Domain %s is active, abort..." % domain_name )
                        print( "To stop:  virsh -c %s destroy %s " % ( uri , domain_name ) )
                        exit(0)
        domain.undefine()

def startDomain(conn, auto_destroy, domain_name, xml_desc):
        print( "Starting %s...\n%s" % ( domain_name, xml_desc ) )
        destroyDomain(conn, auto_destroy, domain_name)
        try:
                conn.defineXML(xml_desc)
                domain = conn.lookupByName(domain_name)
                domain.create()
                print( "Starting domain %s..." % domain_name )
                print( "To connect to the console:  virsh -c %s console %s" % ( uri, domain_name ) )
                print( "To stop:  virsh -c %s destroy %s" % ( uri, domain_name ) )
        except Exception as e:
                print( e )
                exit(1)

def make_nw_spec(network_name, bridge_nw_interface, network, auto_assign_ip):
        spec = '<network>'
        spec += '<name>' + network_name + '</name>'
        spec += '<bridge name="' + bridge_nw_interface + '"/>'
        spec += '<forward/>'
        spec += '<ip address="' + network + '" netmask="255.255.255.0">'
        if auto_assign_ip:
                nw_parts = network.split('.')
                nw_parts[-1] = "2"
                start_dhcp = '.'.join(nw_parts)
                nw_parts[-1] = "254"
                end_dhcp = '.'.join(nw_parts)
                spec += '<dhcp>'
                spec += '<range start="' + start_dhcp + '" end="' + end_dhcp + '"/>'
                spec += '</dhcp>'
        spec += '</ip>'
        spec += '</network>'
        return spec

def make_spec(name, network, kernel, disk, bridge_nw_interface, emulator, auto_assign_ip, ip):
        if not os.path.exists(kernel):
                print( "Kernel image %s does not exist!" % kernel )
                exit(1)
        if not os.path.exists(disk):
                print( "Disk %s does not exist!" % disk )
                exit(1)
        if auto_assign_ip:
                ip_spec = 'dhcp'
        else:
                ip_spec = ip + '::' + network + ':255.255.255.0:' + name + ':eth0:off'
        spec = '<domain type=\'kvm\'>'
        spec += '   <name>' + name + '</name>'
        spec += '   <memory>4096000</memory>'
        spec += '   <currentMemory>4096000</currentMemory>'
        spec += '   <vcpu cpuset=\'1\'>1</vcpu>'
        spec += '   <cpu>'
        spec += '     <model>kvm64</model>'
        spec += '   </cpu>'
        spec += '   <os>'
        spec += '     <type arch=\'x86_64\' machine=\'pc\'>hvm</type>'
        spec += '     <kernel>' + kernel + '</kernel>'
        spec += '     <boot dev=\'hd\'/>'
        spec += '     <cmdline>root=/dev/vda rw console=ttyS0 ip=' + ip_spec + '</cmdline>'
        spec += '   </os>'
        spec += '   <features>'
        spec += '     <acpi/>'
        spec += '     <apic/>'
        spec += '     <pae/>'
        spec += '   </features>'
        spec += '   <clock offset=\'utc\'/>'
        spec += '   <on_poweroff>destroy</on_poweroff>'
        # spec += '   <on_reboot>destroy</on_reboot>'
        spec += '   <on_crash>destroy</on_crash>'
        spec += '   <devices>'
        spec += '     <emulator>' + emulator + '</emulator>'
        spec += '     <disk type=\'file\' device=\'disk\'>'
        spec += '       <source file=\'' + disk + '\'/>'
        spec += '       <target dev=\'vda\' bus=\'virtio\'/>'
        spec += '     </disk>'
        spec += '     <interface type=\'bridge\'>'
        spec += '       <source bridge=\'' + bridge_nw_interface + '\'/>'
        spec += '       <model type=\'virtio\' />'
        spec += '     </interface>'
        spec += '     <serial type=\'pty\'>'
        spec += '      <target port=\'0\'/>'
        spec += '      <alias name=\'serial0\'/>'
        spec += '     </serial>'
        spec += '    <console type=\'pty\'>'
        spec += '      <target type=\'serial\' port=\'0\'/>'
        spec += '      <alias name=\'serial0\'/>'
        spec += '    </console>'
        spec += '   </devices>'
        spec += '</domain>'
        return spec

def getConfig(config, section, key):
        try:
                return os.path.expandvars(config.get(section, key))
        except:
                print( "Configuration file error! Missing item (section: %s, key: %s)" % ( section, key ) )
                exit(1)

# does the user have access to libvirt?
eligible_groups = [ "libvirt", "libvirtd" ]
eligible_user = False
euid = os.geteuid()
if euid == 0:
        eligible_user = True
else:
        username = pwd.getpwuid(euid)[0]
        groups = [g.gr_name for g in grp.getgrall() if username in g.gr_mem]
        for v in eligible_groups:
                if v in groups:
                        eligible_user = True

checkPackages()

if not eligible_user:
        sys.stderr.write("You need to be the 'root' user or in group [" + '|'.join(eligible_groups) + "] to run this script.\n")
        exit(1)

if len(sys.argv) != 3:
        sys.stderr.write("Usage: "+sys.argv[0]+" [config file] [start|stop|restart]\n")
        sys.exit(1)

if not os.path.exists(sys.argv[1]):
        sys.stderr.write("Error: config file \"" + sys.argv[1] + "\" was not found!\n")
        sys.exit(1)

command = sys.argv[2]
command_options = ["start", "stop", "restart"]
if not command in command_options:
        sys.stderr.write("Usage: "+sys.argv[0]+" [config file] [start|stop|restart]\n")
        sys.exit(1)

Config = ConfigParser.ConfigParser()
Config.read(sys.argv[1])

network_addr = getConfig(Config, "main", "network")
getConfig(Config, "main", "auto_destroy")
auto_destroy = Config.getboolean("main", "auto_destroy")
getConfig(Config, "main", "auto_assign_ip")
auto_assign_ip = Config.getboolean("main", "auto_assign_ip")
network_name = 'ops_default'
uri = 'qemu:///system'

# Connect to libvirt
conn = libvirt.open(uri)
if conn is None:
        print( "Failed to open connection to the hypervisor" )
        exit(1)

if command == "start":
        destroyNetwork(conn, network_name)

        # Change the default bridge device from virbr0 to virbr%d.
        # This will make libvirt try virbr0, virbr1, etc. until it finds a free one.
        cnt = 0
        ifaces = networkInterfaces()
        found_virbr = False
        while found_virbr == False:
                if cnt > 254:
                        print( "Giving up on looking for a free virbr network interface!" )
                        exit(1)
                bridge_nw_interface = 'virbr' + str(cnt)
                if bridge_nw_interface not in ifaces:
                        print( "bridge_nw_interface: %s" % bridge_nw_interface )
                        network_spec = make_nw_spec(network_name, bridge_nw_interface, network_addr, auto_assign_ip)
                        try:
                                conn.networkDefineXML(network_spec)
                                nw = conn.networkLookupByName(network_name)
                                nw.create()
                                found_virbr = True
                        except:
                                print( "Network Name: %s" % network_name )
                                destroyNetwork( conn, network_name )
                                print( "Error creating network interface" )
                cnt += 1
else:
        # verify network exists
        try:
                nw = conn.networkLookupByName(network_name)
        except:
                print( "Error! Virtual network " + network_name + " is not defined!" )
                exit(1)
        if not nw.isActive():
                print( "Error! Virtual network " + network_name + " is not running!" )
                exit(1)

emulator = getConfig(Config, "main", "emulator")
if not os.path.exists(emulator):
        print( "Emulator %s does not exist!" % emulator )
        exit(1)

controller_name = 'controller'
if command == "start":
        # Define the controller xml
        controller_kernel = getConfig(Config, "controller", "kernel")
        controller_disk = getConfig(Config, "controller", "disk")

        controller_ip = None
        if not auto_assign_ip:
                controller_ip = getConfig(Config, "controller", "ip")
        controller_spec = make_spec(controller_name, network_addr, controller_kernel,
                                controller_disk, bridge_nw_interface, emulator,
                                auto_assign_ip, controller_ip)

        # Now that network is setup let's actually run the virtual images
        startDomain(conn, auto_destroy, controller_name, controller_spec)
elif command == "stop":
        destroyDomain(conn, True, controller_name)
elif command == "restart":
        restartDomain(conn, controller_name)

for i in Config.sections():
        if i.startswith("compute"):
                if command == "start":
                        # Define the compute xml
                        kernel = getConfig(Config, i, "kernel")
                        disk = getConfig(Config, i, "disk")
                        compute_ip = None
                        if not auto_assign_ip:
                                compute_ip = getConfig(Config, i, "ip")
                        spec = make_spec(i, network_addr, kernel, disk, bridge_nw_interface,
                                        emulator, auto_assign_ip, compute_ip)
                        startDomain(conn, auto_destroy, i, spec)
                elif command == "stop":
                        destroyDomain(conn, True, i)
                elif command == "restart":
                        restartDomain(conn, i)

conn.close()
