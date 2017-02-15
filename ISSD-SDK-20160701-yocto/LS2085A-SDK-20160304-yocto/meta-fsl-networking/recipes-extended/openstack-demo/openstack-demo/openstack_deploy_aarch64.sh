#!/bin/sh

usage_message() {
	echo "Usage: $0 [-C] [-c] [-n]
	-C: to install controller node services
	-c: to install compute node services
	-n: to install network node services
	-b: to install block storage node services
	-o: to install object storage node services
	"
}


if [ "$#" = "0" ]
then
	usage_message;
	exit 1;
fi

controller_flag=''
compute_flag=''
network_flag=''
block_storage_flag=''
object_storage_flag=''


ctrl_service_list='
    rabbitmq-server
    mysqld
    memcached
    ntpd
    rabbitmq-server
    ceilometer-agent-central
    ceilometer-agent-notification
    ceilometer-alarm-evaluator
    ceilometer-alarm-notifier
    ceilometer-api
    ceilometer-collector
    cinder-api
    cinder-scheduler
    glance-api
    glance-registry
    heat-api
    heat-api-cfn
    heat-engine
    horizon
    keystone
    neutron-server
    nova-api
    nova-scheduler
    nova-conductor
    nova-cert
    nova-consoleauth
    nova-serialproxy
    apache2
'

cinder_service_list='
    tgtd
    cinder-backup
    cinder-volume
'

swift_service_list='
    swift
'

compute_service_list='
    libvirtd
    nova-compute
    nova-novncproxy
    nova-spicehtml5proxy
    ceilometer-agent-compute
    openvswitch-switch
    neutron-openvswitch-agent
    iscsid
'

network_service_list='
    dnsmasq
    neutron-dhcp-agent
    neutron-l3-agent
    neutron-metadata-agent
    openvswitch-switch
    neutron-openvswitch-agent
'

cp -f openstack_conf/controller/nova/nova-serialproxy /etc/init.d/
#for service in $ctrl_service_list $network_service_list $compute_service_list $cinder_service_list $swift_service_list $compute_service_list
#do
#    chkconfig --del $service || sed '2 i# chkconfig:   2345 50 10'   -i /etc/init.d/$service && chkconfig --del $service
#done
#
#killall -9 python
rm -r /var/log && mkdir /var/log && cp -r /var/volatile/log/* /var/log/
cp -r openstack_conf /tmp/ && cd /tmp/
sed -i '87s/^ /#/g' /usr/lib/python2.7/site-packages/nova/openstack/common/sslutils.py

while getopts Ccnboh opt_value
do
	case $opt_value in
		C) controller_flag='true';
			;;
		c) compute_flag='true';
			;;
		n) network_flag='true';
			;;
		b) block_storage_flag='true';
			;;
		o) object_storage_flag='true';
			;;
		?) usage_message;exit 1;
			;;
	esac
done

if [ -n "$controller_flag" ]
then
	read -p "Controller Node Management Interface IP: " controller_ip
	read -p "neutron_metadata_proxy_shared_secret: " metadata_secret
	if [ "$controller_ip" = "" ]; then
	echo "ERROR:No Ip Address for Management Interface"
	exit
	fi

        for service in $ctrl_service_list
        do
           chkconfig --add $service
        done

	cd openstack_conf/controller || exit 0
	find . -type f | xargs perl -p -i -e "s|openstack-controller|$controller_ip|g"
	find . -type f | xargs perl -p -i -e "s|METADATA_SECRET|$metadata_secret|g"

	read -p "Mysql server User :" myuser
	read -p "Mysql server Password:" mypass
        /etc/init.d/mysqld stop


        mysql_install_db 
	cp mysql/my.cnf /etc/my.cnf
	/etc/init.d/mysqld restart
	sleep 10
        mysqladmin -uroot password $mypass

	mysql -u$myuser -p$mypass <<EOF
	drop database keystone;
	drop database glance;
	drop database nova;
	drop database neutron;
	drop database cinder;
	drop database heat;
EOF

	#create database for keystone, glance, nova, neutron
	mysql -u$myuser -p$mypass <<EOF
	CREATE DATABASE keystone;
	GRANT ALL PRIVILEGES ON keystone.* TO 'keystone'@'localhost' IDENTIFIED BY 'KEYSTONE_DBPASS';
	GRANT ALL PRIVILEGES ON keystone.* TO 'keystone'@'%' IDENTIFIED BY 'KEYSTONE_DBPASS';
	GRANT ALL PRIVILEGES ON keystone.* TO 'keystone'@'$controller_ip' IDENTIFIED BY 'KEYSTONE_DBPASS';
	CREATE DATABASE glance;
	GRANT ALL PRIVILEGES ON glance.* TO 'glance'@'localhost' IDENTIFIED BY 'GLANCE_DBPASS';
	GRANT ALL PRIVILEGES ON glance.* TO 'glance'@'%' IDENTIFIED BY 'GLANCE_DBPASS';
	GRANT ALL PRIVILEGES ON glance.* TO 'glance'@'$controller_ip' IDENTIFIED BY 'GLANCE_DBPASS';
	CREATE DATABASE nova;
	GRANT ALL PRIVILEGES ON nova.* TO 'nova'@'localhost' IDENTIFIED BY 'NOVA_DBPASS';
	GRANT ALL PRIVILEGES ON nova.* TO 'nova'@'%' IDENTIFIED BY 'NOVA_DBPASS';
	GRANT ALL PRIVILEGES ON nova.* TO 'nova'@'$controller_ip' IDENTIFIED BY 'NOVA_DBPASS';
	CREATE DATABASE neutron;
	GRANT ALL PRIVILEGES ON neutron.* TO 'neutron'@'localhost' IDENTIFIED BY 'NEUTRON_DBPASS';
	GRANT ALL PRIVILEGES ON neutron.* TO 'neutron'@'%' IDENTIFIED BY 'NEUTRON_DBPASS';
	GRANT ALL PRIVILEGES ON neutron.* TO 'neutron'@'$controller_ip' IDENTIFIED BY 'NEUTRON_DBPASS';
	CREATE DATABASE cinder;
	GRANT ALL PRIVILEGES ON cinder.* TO 'cinder'@'localhost' IDENTIFIED BY 'CINDER_DBPASS';
	GRANT ALL PRIVILEGES ON cinder.* TO 'cinder'@'%' IDENTIFIED BY 'CINDER_DBPASS';
	GRANT ALL PRIVILEGES ON cinder.* TO 'cinder'@'$controller_ip' IDENTIFIED BY 'CINDER_DBPASS';
        CREATE DATABASE heat;
        GRANT ALL PRIVILEGES ON heat.* TO 'heat'@'localhost' IDENTIFIED BY 'HEAT_DBPASS';
        GRANT ALL PRIVILEGES ON heat.* TO 'heat'@'%' IDENTIFIED BY 'HEAT_DBPASS';
        GRANT ALL PRIVILEGES ON heat.* TO 'heat'@'$controller_ip' IDENTIFIED BY 'HEAT_DBPASS';
	FLUSH PRIVILEGES;
EOF

	# Change Keystone configuration file
	cp -f keystone/keystone.conf /etc/keystone/keystone.conf

	keystone-manage db_sync

	/etc/init.d/keystone restart
	unset OS_PASSWORD OS_AUTH_URL OS_USERNAME OS_TENANT_NAME SERVICE_ENDPOINT SERVICE_TOKEN
	export OS_SERVICE_TOKEN=ADMIN_TOKEN
	export OS_SERVICE_ENDPOINT=http://$controller_ip:35357/v2.0
	export OS_USERNAME=admin
	export OS_PASSWORD=admin
	export OS_TENANT_NAME=admin
	export OS_AUTH_URL=http://$controller_ip:35357/v2.0
	sleep 3

	#create an administrative user
	keystone user-create --name=admin --pass=admin --email=ADMIN_EMAIL
	keystone role-create --name=admin
	keystone tenant-create --name=admin --description="Admin Tenant"
	keystone user-role-add --user=admin --tenant=admin --role=admin
	keystone user-role-add --user=admin --role=_member_ --tenant=admin

	#create a normal user
	keystone user-create --name=demo --pass=DEMO_PASS --email=DEMO_EMAIL
	keystone tenant-create --name=demo --description="Demo Tenant"
	keystone user-role-add --user=demo --role=_member_ --tenant=demo

	#create a service tenant
	keystone tenant-create --name=service --description="Service Tenant"

	#Define services and API endpoints
	keystone service-create --name=keystone --type=identity --description="OpenStack Identity"
	keystone endpoint-create --service-id=$(keystone service-list | awk '/ identity / {print $2}') \
	--publicurl=http://$controller_ip:5000/v2.0 --internalurl=http://$controller_ip:5000/v2.0 \
	--adminurl=http://$controller_ip:35357/v2.0

	cp -f glance/glance-api.conf /etc/glance/glance-api.conf
	cp -f glance/glance-registry.conf /etc/glance/glance-registry.conf
	glance-manage db_sync
	sleep 10
	keystone user-create --name=glance --pass=GLANCE_PASS --email=glance@example.com
	keystone user-role-add --user=glance --tenant=service --role=admin
	keystone service-create --name=glance --type=image --description="OpenStack Image Service"
	keystone endpoint-create --service-id=$(keystone service-list | awk '/ image / {print $2}') \
	--publicurl=http://$controller_ip:9292 --internalurl=http://$controller_ip:9292 --adminurl=http://$controller_ip:9292

	/etc/init.d/glance-registry restart
	/etc/init.d/glance-api restart

	#Nova configuration
	cp -f nova/nova.conf /etc/nova/nova.conf
	nova-manage db sync
	sleep 10

	keystone user-create --name=nova --pass=NOVA_PASS --email=nova@example.com
	keystone user-role-add --user=nova --tenant=service --role=admin
	keystone service-create --name=nova --type=compute --description="OpenStack Comute"
	keystone endpoint-create --service-id=$(keystone service-list | awk '/ compute / {print $2}') \
	--publicurl=http://$controller_ip:8774/v2/%\(tenant_id\)s --internalurl=http://$controller_ip:8774/v2/%\(tenant_id\)s \
	--adminurl=http://$controller_ip:8774/v2/%\(tenant_id\)s
	/etc/init.d/nova-api restart
	/etc/init.d/nova-scheduler restart
	/etc/init.d/nova-conductor restart

	#Install network service
	keystone user-create --name=neutron --pass=NEUTRON_PASS --email=neutron@example.com
	keystone user-role-add --user=neutron --tenant=service --role=admin
	keystone service-create --name=neutron --type=network --description="OpenStack Networking"
	keystone endpoint-create --service-id=$(keystone service-list | awk '/ network / {print $2}') \
	--publicurl=http://$controller_ip:9696 --adminurl=http://$controller_ip:9696 --internalurl=http://$controller_ip:9696
	
	cp -f neutron/neutron.conf /etc/neutron/neutron.conf
	cp -f neutron/ml2_conf.ini /etc/neutron/plugins/ml2/
	neutron-db-manage upgrade head
	/etc/init.d/neutron-server restart

	/etc/init.d/nova-api restart
	/etc/init.d/nova-serialproxy restart
	/etc/init.d/nova-scheduler restart
	/etc/init.d/nova-conductor restart
	/etc/init.d/neutron-server restart
	
	#Install cinder storage service
	cp -f cinder/cinder.conf /etc/cinder/cinder.conf
	cinder-manage db sync
	keystone user-create --name=cinder --pass=CINDER_PASS --email=cinder@example.com
	keystone user-role-add --user=cinder --tenant=service --role=admin
	keystone service-create --name=cinder --type=volume --description="OpenStack Block Storage"
	keystone endpoint-create --service-id=$(keystone service-list | awk '/ volume / {print $2}') \
	--publicurl=http://$controller_ip:8776/v1/%\(tenant_id\)s \
	--internalurl=http://$controller_ip:8776/v1/%\(tenant_id\)s \
	--adminurl=http://$controller_ip:8776/v1/%\(tenant_id\)s
	
	keystone service-create --name=cinderv2 --type=volumev2 --description="OpenStack Block Storage v2"
	keystone endpoint-create --service-id=$(keystone service-list | awk '/ volumev2 / {print $2}') \
	--publicurl=http://$controller_ip:8776/v2/%\(tenant_id\)s \
	--internalurl=http://$controller_ip:8776/v2/%\(tenant_id\)s \
	--adminurl=http://$controller_ip:8776/v2/%\(tenant_id\)s
	
	/etc/init.d/cinder-scheduler restart
	/etc/init.d/cinder-api restart

	keystone user-create --name=swift --pass=SWIFT_PASS --email=swift@example.com
	keystone user-role-add --user=swift --tenant=service --role=admin
	keystone service-create --name=swift --type=object-store --description="OpenStack Object Storage"
	keystone endpoint-create --service-id=$(keystone service-list | awk '/ object-store / {print $2}') \
	--publicurl="http://$controller_ip:8080/v1/AUTH_%(tenant_id)s" \
	--internalurl="http://$controller_ip:8080/v1/AUTH_%(tenant_id)s" \
	--adminurl=http://$controller_ip:8080

	cp -f swift/swift.conf /etc/swift/swift.conf
	cp -f swift/proxy-server.conf /etc/swift/proxy-server.conf

        keystone user-create --name=heat --pass=HEAT_PASS --email=ADMIN_EMAIL
        keystone role-create --name=heat_stack_owner
        keystone user-role-add --user=heat --role=admin --tenant=service
        keystone user-role-add --user=demo --role=heat_stack_owner --tenant=demo
        keystone role-create --name=heat_stack_user
        
        keystone service-create --name=heat --type=orchestration --description="OpenStack Orchestration"
        keystone service-create --name=heat --type=cloudformation --description="OpenStack Orchestration cloudformation"
        keystone endpoint-create  \
        	          --service-id=$(keystone service-list | awk '/ orchestration / {print $2}') \
        	          --publicurl http://$controller_ip:8004/v1/%\(tenant_id\)s \
        		  --internalurl http://$controller_ip:8004/v1/%\(tenant_id\)s \
        		  --adminurl http://$controller_ip:8004/v1/%\(tenant_id\)s \
        
        keystone endpoint-create  \
        	  --service-id=$(keystone service-list | awk '/ cloudformation/ {print $2}') \
        	  --publicurl http://$controller_ip:8000/v1 \
        	  --internalurl http://$controller_ip:8000/v1 \
        	  --adminurl http://$controller_ip:8000/v1 \
        
        heat-keystone-setup-domain \
        	  --stack-user-domain-name heat_user_domain \
        	  --stack-domain-admin heat_domain_admin \
        	  --stack-domain-admin-password HEAT_DOMAIN_PASS
        
        heat-manage db_sync
        cp -f heat/heat.conf /etc/heat/heat.conf

	mkdir -p /var/log/apache2
	mkdir -p /usr/lib/python2.7/site-packages/horizon/static/horizon/lib/angular/
	mkdir -p /usr/lib/python2.7/site-packages/horizon/static/horizon/lib/
	mkdir -p /usr/lib/python2.7/site-packages/horizon/static/horizon/lib/jquery/

	cp -f dashboard/local_settings.py /etc/openstack-dashboard/
	cp -f dashboard/openstack-dashboard-apache.conf /etc/apache2/conf.d/
	cp -f dashboard/angular.js /usr/lib/python2.7/site-packages/horizon/static/horizon/lib/angular/
	cp -f dashboard/term.js /usr/lib/python2.7/site-packages/horizon/static/horizon/lib/
	cp -f dashboard/jquery.js /usr/lib/python2.7/site-packages/horizon/static/horizon/lib/jquery/
	/etc/init.d/apache2 restart

	cd -
	rm openstack_conf -rf

        for service in $ctrl_service_list
        do
           chkconfig --add $service
	   /etc/init.d/$service restart
        done

cat > /etc/nova/openrc <<EOF
export OS_SERVICE_TOKEN=ADMIN_TOKEN
export OS_SERVICE_ENDPOINT=http://$controller_ip:35357/v2.0
export OS_USERNAME=admin
export OS_PASSWORD=admin
export OS_TENANT_NAME=admin
export OS_AUTH_URL=http://$controller_ip:35357/v2.0
EOF

        source /etc/nova/openrc

fi

if [ -n "$compute_flag" ]
then
	read -p "Controller Node Management Interface IP: " controller_ip
	read -p "Compute Node Management Interface IP: " my_ip
	read -p "Compute Node Data Interface IP: " data_ip

	unset OS_PASSWORD OS_AUTH_URL OS_USERNAME OS_TENANT_NAME SERVICE_ENDPOINT SERVICE_TOKEN
	export OS_USERNAME=admin
	export OS_PASSWORD=ADMIN_PASS
	export OS_TENANT_NAME=admin
	export OS_AUTH_URL=http://$controller_ip:35357/v2.0
	
	cd openstack_conf/compute || exit 0
	find . -type f | xargs perl -p -i -e "s|openstack-controller|$controller_ip|g"
	find . -type f | xargs perl -p -i -e "s|tunnel-ip|$data_ip|g"
	find . -type f | xargs perl -p -i -e "s|MY_IP|$my_ip|g"
	
	cp -f nova/nova.conf /etc/nova/nova.conf
	cp -f neutron/neutron.conf /etc/neutron/neutron.conf
	cp -f neutron/ml2_conf.ini /etc/neutron/plugins/ml2/
	nova-manage db sync

	/etc/init.d/openvswitch-switch restart
	/etc/init.d/nova-compute restart
	/etc/init.d/neutron-openvswitch-agent restart

	cd -
	rm openstack_conf -rf

        for service in $compute_service_list
        do
           chkconfig --add $service
	   /etc/init.d/$service restart
        done
fi

if [ -n "$network_flag" ]
then
	read -p "Controller Node Management Interface IP: " controller_ip
	read -p "Network Node Data Interface IP: " data_ip
	read -p "metadata_proxy_shared_secret, this secret must be the same with controller node neutron_metadata_proxy_shared_secret:" metadata_secret unset OS_PASSWORD OS_AUTH_URL OS_USERNAME OS_TENANT_NAME SERVICE_ENDPOINT SERVICE_TOKEN
	export OS_USERNAME=admin
	export OS_PASSWORD=ADMIN_PASS
	export OS_TENANT_NAME=admin
	export OS_AUTH_URL=http://$controller_ip:35357/v2.0
	
	cd openstack_conf/network || exit 0
	find . -type f | xargs perl -p -i -e "s|openstack-controller|$controller_ip|g"
	find . -type f | xargs perl -p -i -e "s|tunnel-ip|$data_ip|g"
	find . -type f | xargs perl -p -i -e "s|METADATA_SECRET|$metadata_secret|g"
	
	cp -f neutron/neutron.conf /etc/neutron/nertron.conf
	cp -f neutron/metadata_agent.ini /etc/neutron/metadata_agent.ini
	cp -f neutron/ml2_conf.ini /etc/neutron/plugins/ml2/

	/etc/init.d/openvswitch-switch restart
	ovs-vsctl add-br br-ex
	read -p "Input your network node physical external network interface: " interface_name
	ovs-vsctl add-port br-ex $interface_name
	
	/etc/init.d/neutron-openvswitch-agent restart
	/etc/init.d/neutron-l3-agent restart
	/etc/init.d/neutron-dhcp-agent restart
	/etc/init.d/neutron-metadata-agent restart
	
	cd -
	rm openstack_conf -rf
        for service in $network_service_list
        do
           chkconfig --add $service
	   /etc/init.d/$service restart
        done
fi

if [ -n "$object_storage_flag" ]
then
	read -p "Object Storage Node Management Interface IP: " object_ip
	read -p "The path of disk mount location,like /srv/node if your mount location is /srv/node/sdb1:" mount_path

	cd openstack_conf/swift || exit 0
	find . -type f | xargs perl -p -i -e "s|Object-IP|$object_ip|g"
	find . -type f | xargs perl -p -i -e "s|Disk_mount_path|$mount_path|g"
	cp -f account-server.conf /etc/swift/account-server.conf
	cp -f container-server.conf /etc/swift/container-server.conf
	cp -f object-server.conf /etc/swift/object-server.conf
	cp -f swift.conf /etc/swift/swift.conf
	cp -f rsyncd.conf /etc/rsyncd.conf
        for service in $swift_service_list
        do
            chkconfig --add $service
	   /etc/init.d/$service restart
        done

	cd -
	rm openstack_conf -rf
fi

if [ -n "$block_storage_flag" ]
then
	read -p "Controller Node Management Interface IP: " controller_ip
	read -p "Local Management Interface IP: " my_ip
    mkdir -p /var/log/cinder

    sed -e 's:openstack-controller:'$controller_ip':g' -i openstack_conf/cinder/cinder.conf
    sed -e 's:openstack-block-storage:'$my_ip':g' -i openstack_conf/cinder/cinder.conf
    cp -f openstack_conf/cinder/cinder.conf /etc/cinder/

    for service in $cinder_service_list
    do
        chkconfig --add $service
	   /etc/init.d/$service restart
    done
    rm openstack_conf -rf
fi
