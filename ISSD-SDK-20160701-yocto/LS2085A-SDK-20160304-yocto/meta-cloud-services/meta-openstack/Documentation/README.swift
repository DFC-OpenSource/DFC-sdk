Summary
=======

This document is not intended to provide detail of how Swift in general
works, but rather it highlights the details of how Swift cluster is
setup and OpenStack is configured to allow various Openstack components
interact with Swift.


Swift Overview
==============

Openstack Swift is an object storage service.  Clients can access swift
objects through RESTful APIs.  Swift objects can be grouped into a
"container" in which containers are grouped into "account".  Each account
or container in Swift cluster is represented by a SQLite database which
contains information related to that account or container.  Each
Swift object is just user data input.

Swift cluster can include a massive number of storage devices.  Any Swift
storage device can be configured to store container databases and/or
account databases and/or objects.  Each Swift account database can be
identified by tuple (account name).  Each Swift container database can
be identified by tuple (account name, container name).  Each swift object
can be identified by tuple (account name, container name, object name).

Swift uses "ring" static mapping algorithm to identify what storage device
hosting account database, container database, or object contain (similar
to Ceph uses Crush algorithm to identify what OSD hosting Ceph object).
A Swift cluster has 3 rings (account ring, container ring, and object ring)
used for finding location of account database, container database, or
object file respectively.

Swift service includes the following core services: proxy-server which
provides the RESTful APIs for Swift clients to access; account-server
which manages accounts; container-server which manages containers;
and object-server which manages objects.


Swift Cluster Setup
===================

The Swift default cluster is setup to have the followings:

* All Swift main process services including proxy-server, account-server
  container-server, object-server run on Controller node.
* 3 zones in which each zone has only 1 storage device.
  The underneath block devices for these 3 storage devices are loopback
  devices. The size of the backing up loopback files is 2Gbytes by default
  and can be changed at compile time through variable SWIFT_BACKING_FILE_SIZE.
  If SWIFT_BACKING_FILE_SIZE="0G" then is for disabling loopback devices
  and using local filesystem as Swift storage device.
* All 3 Swift rings have 2^12 partitions and 2 replicas.

The Swift default cluster is mainly for demonstration purpose.  One might
wants to have a different Swift cluster setup than this setup (e.g. using
real hardware block device instead of loopback devices).

The script /etc/swift/swift_setup.sh is provided to ease the task of setting
up a complicated Swift cluster.  It reads a cluster config file, which describes
what storage devices are included in what rings, and constructs the cluster.

For details of how to use swift_setup.sh and the format of Swift cluster
config file please refer to the script's help:

    $ swift_setup.sh


Glance and Swift
================

Glance can store images into Swift cluster when "default_store = swift"
is set in /etc/glance/glance-api.conf.

By default "default_store" has value of "file" which tells Glance to
store images into local filesystem.  "default_store" value can be set
during compile time through variable GLANCE_DEFAULT_STORE.

The following configuration options in /etc/glance/glance-api.conf affect
on how glance interacts with Swift cluster:

    swift_store_auth_version = 2
    swift_store_auth_address = http://127.0.0.1:5000/v2.0/
    swift_store_user = service:glance
    swift_store_key = password
    swift_store_container = glance
    swift_store_create_container_on_put = True
    swift_store_large_object_size = 5120
    swift_store_large_object_chunk_size = 200
    swift_enable_snet = False

With these default settings, the images will be stored under
Swift account: "service" tenant ID and Swift cluster container:
"glance".


Cinder Backup and Swift
=======================

Cinder-backup has ability to store volume backup into Swift
cluster with the following command:

    $ cinder backup-create <cinder volume ID>

where <cinder volume ID> is ID of an existing Cinder volume if
the configure option "backup_driver" in /etc/cinder/cinder.conf
is set to "cinder.backup.drivers.ceph".

Cinder-backup is not be able to create a backup for any cinder
volume which backed by NFS or Glusterfs.  This is because NFS
and Gluster cinder-volume backend drivers do not support the
backup functionality.  In other words, only cinder volume
backed by lvm-iscsi and ceph-rbd are able to be backed-up
by cinder-backup.

The following configuration options in /etc/cinder/cinder.conf affect
on how cinder-backup interacts with Swift cluster:

    backup_swift_url=http://controller:8888/v1/AUTH_
    backup_swift_auth=per_user
    #backup_swift_user=<None>
    #backup_swift_key=<None>
    backup_swift_container=cinder-backups
    backup_swift_object_size=52428800
    backup_swift_retry_attempts=3
    backup_swift_retry_backoff=2
    backup_compression_algorithm=zlib

With these defaults settings, the tenant ID of the keystone user that
runs "cinder backup-create" command will be used as Swift cluster
account name, along with "cinder-backups" Swift cluster container name
in which the volume backups will be saved into.


Build Configuration Options
===========================

To add/deploy swift as part of a controller build, add the following layer
to your configuration: meta-cloud-services/meta-openstack-swift-deploy

Test Steps
==========

This section describes test steps and expected results to demonstrate that
Swift is integrated properly into OpenStack.

Please note: the following commands are carried on Controller node, unless
otherwise explicitly indicated.

    $ Start Controller and Compute node
    $ . /etc/nova/openrc
    $ dd if=/dev/urandom of=50M_c1.org bs=1M count=50
    $ dd if=/dev/urandom of=50M_c2.org bs=1M count=50
    $ dd if=/dev/urandom of=100M_c2.org bs=1M count=100
    $ swift upload c1 50M_c1.org && swift upload c2 50M_c2.org && swift upload c2 100M_c2.org
    $ swift list

c1
c2

    $ swift stat c1

       Account: AUTH_4ebc0e00338f405c9267866c6b984e71
     Container: c1
       Objects: 1
         Bytes: 52428800
      Read ACL:
     Write ACL:
       Sync To:
      Sync Key:
 Accept-Ranges: bytes
   X-Timestamp: 1396457818.76909
    X-Trans-Id: tx0564472425ad47128b378-00533c41bb
  Content-Type: text/plain; charset=utf-8

(Should see there is 1 object)

    $ swift stat c2

root@controller:~# swift stat c2
       Account: AUTH_4ebc0e00338f405c9267866c6b984e71
     Container: c2
       Objects: 2
         Bytes: 157286400
      Read ACL:
     Write ACL:
       Sync To:
      Sync Key:
 Accept-Ranges: bytes
   X-Timestamp: 1396457826.26262
    X-Trans-Id: tx312934d494a44bbe96a00-00533c41cd
  Content-Type: text/plain; charset=utf-8

(Should see there are 2 objects)

    $ swift stat c3

Container 'c3' not found

    $ mv 50M_c1.org 50M_c1.save && mv 50M_c2.org 50M_c2.save && mv 100M_c2.org 100M_c2.save
    $ swift download c1 50M_c1.org && swift download c2 50M_c2.org && swift download c2 100M_c2.org
    $ md5sum 50M_c1.save 50M_c1.org && md5sum 50M_c2.save 50M_c2.org && md5sum 100M_c2.save 100M_c2.org

a8f7d671e35fcf20b87425fb39bdaf05  50M_c1.save
a8f7d671e35fcf20b87425fb39bdaf05  50M_c1.org
353233ed20418dbdeeb2fad91ba4c86a  50M_c2.save
353233ed20418dbdeeb2fad91ba4c86a  50M_c2.org
3b7cbb444c2ba93819db69ab3584f4bd  100M_c2.save
3b7cbb444c2ba93819db69ab3584f4bd  100M_c2.org

(The md5sums of each pair "zzz.save" and "zzz.org" files must be the same)

    $ swift delete c1 50M_c1.org && swift delete c2 50M_c2.org
    $ swift stat c1

       Account: AUTH_4ebc0e00338f405c9267866c6b984e71
     Container: c1
       Objects: 0
         Bytes: 0
      Read ACL:
     Write ACL:
       Sync To:
      Sync Key:
 Accept-Ranges: bytes
   X-Timestamp: 1396457818.77284
    X-Trans-Id: tx58e4bb6d06b84276b8d7f-00533c424c
  Content-Type: text/plain; charset=utf-8

(Should see there is no object)

    $ swift stat c2

       Account: AUTH_4ebc0e00338f405c9267866c6b984e71
     Container: c2
       Objects: 1
         Bytes: 104857600
      Read ACL:
     Write ACL:
       Sync To:
      Sync Key:
 Accept-Ranges: bytes
   X-Timestamp: 1396457826.25872
    X-Trans-Id: txdae8ab2adf4f47a4931ba-00533c425b
  Content-Type: text/plain; charset=utf-8

(Should see there is 1 object)

    $ swift upload c1 50M_c1.org && swift upload c2 50M_c2.org
    $ rm *.org
    $ swift download c1 50M_c1.org && swift download c2 50M_c2.org && swift download c2 100M_c2.org
    $ md5sum 50M_c1.save 50M_c1.org && md5sum 50M_c2.save 50M_c2.org && md5sum 100M_c2.save 100M_c2.org

31147c186e7dd2a4305026d3d6282861  50M_c1.save
31147c186e7dd2a4305026d3d6282861  50M_c1.org
b9043aacef436dfbb96c39499d54b850  50M_c2.save
b9043aacef436dfbb96c39499d54b850  50M_c2.org
b1a1b6b34a6852cdd51cd487a01192cc  100M_c2.save
b1a1b6b34a6852cdd51cd487a01192cc  100M_c2.org

(The md5sums of each pair "zzz.save" and "zzz.org" files must be the same)

    $ neutron net-create mynetwork
    $ glance image-create --name myfirstimage --is-public true --container-format bare --disk-format qcow2 --file /root/images/cirros-*-x86_64-disk.img
    $ glance image-list

+--------------------------------------+--------------+-------------+------------------+---------+--------+
| ID                                   | Name         | Disk Format | Container Format | Size    | Status |
+--------------------------------------+--------------+-------------+------------------+---------+--------+
| 79f52103-5b22-4aa5-8159-2d146b82b0b2 | myfirstimage | qcow2       | bare             | 9761280 | active |
+--------------------------------------+--------------+-------------+------------------+---------+--------+

    $ export OS_TENANT_NAME=service && export OS_USERNAME=glance
    $ swift list glance

79f52103-5b22-4aa5-8159-2d146b82b0b2

(The object name in the "glance" container must be the same as glance image id just created)

    $ swift download glance 79f52103-5b22-4aa5-8159-2d146b82b0b2
    $ md5sum 79f52103-5b22-4aa5-8159-2d146b82b0b2 /root/images/cirros-*-x86_64-disk.img

50bdc35edb03a38d91b1b071afb20a3c  79f52103-5b22-4aa5-8159-2d146b82b0b2
50bdc35edb03a38d91b1b071afb20a3c  /root/images/cirros-*-x86_64-disk.img

(The md5sum of these 2 files must be the same)

    $ ls /etc/glance/images/
(This should be empty)

    $ . /etc/nova/openrc
    $ nova boot --image myfirstimage --flavor 1 myinstance
    $ nova list

+--------------------------------------+------------+--------+------------+-------------+----------+
| ID                                   | Name       | Status | Task State | Power State | Networks |
+--------------------------------------+------------+--------+------------+-------------+----------+
| bc9662a0-0dac-4bff-a7fb-b820957c55a4 | myinstance | ACTIVE | -          | Running     |          |
+--------------------------------------+------------+--------+------------+-------------+----------+

    $ From dashboard, log into VM console to make sure the VM is really running
    $ nova delete bc9662a0-0dac-4bff-a7fb-b820957c55a4
    $ glance image-delete 79f52103-5b22-4aa5-8159-2d146b82b0b2
    $ export OS_TENANT_NAME=service && export OS_USERNAME=glance
    $ swift list glance

(Should be empty)

    $ . /etc/nova/openrc && . /etc/cinder/add-cinder-volume-types.sh
    $ cinder create --volume_type lvm_iscsi --display_name=lvm_vol_1 1
    $ cinder list

+--------------------------------------+-----------+--------------+------+-------------+----------+-------------+
|                  ID                  |   Status  | Display Name | Size | Volume Type | Bootable | Attached to |
+--------------------------------------+-----------+--------------+------+-------------+----------+-------------+
| 3e388ae0-2e20-42a2-80da-3f9f366cbaed | available |  lvm_vol_1   |  1   |  lvm_iscsi  |  false   |             |
+--------------------------------------+-----------+--------------+------+-------------+----------+-------------+

    $ cinder backup-create 3e388ae0-2e20-42a2-80da-3f9f366cbaed
    $ cinder backup-list

+--------------------------------------+--------------------------------------+-----------+------+------+--------------+----------------+
|                  ID                  |              Volume ID               |   Status  | Name | Size | Object Count |   Container    |
+--------------------------------------+--------------------------------------+-----------+------+------+--------------+----------------+
| 1444f5d0-3a87-40bc-a7a7-f3c672768b6a | 3e388ae0-2e20-42a2-80da-3f9f366cbaed | available | None |  1   |      22      | cinder-backups |
+--------------------------------------+--------------------------------------+-----------+------+------+--------------+----------------+

    $ swift list

c1
c2
cinder-backups

(Should see new Swift container "cinder-backup")

    $ swift list cinder-backups

volume_3e388ae0-2e20-42a2-80da-3f9f366cbaed/20140402193029/az_nova_backup_1444f5d0-3a87-40bc-a7a7-f3c672768b6a-00001
volume_3e388ae0-2e20-42a2-80da-3f9f366cbaed/20140402193029/az_nova_backup_1444f5d0-3a87-40bc-a7a7-f3c672768b6a-00002
volume_3e388ae0-2e20-42a2-80da-3f9f366cbaed/20140402193029/az_nova_backup_1444f5d0-3a87-40bc-a7a7-f3c672768b6a-00003
volume_3e388ae0-2e20-42a2-80da-3f9f366cbaed/20140402193029/az_nova_backup_1444f5d0-3a87-40bc-a7a7-f3c672768b6a-00004
volume_3e388ae0-2e20-42a2-80da-3f9f366cbaed/20140402193029/az_nova_backup_1444f5d0-3a87-40bc-a7a7-f3c672768b6a-00005
volume_3e388ae0-2e20-42a2-80da-3f9f366cbaed/20140402193029/az_nova_backup_1444f5d0-3a87-40bc-a7a7-f3c672768b6a-00006
volume_3e388ae0-2e20-42a2-80da-3f9f366cbaed/20140402193029/az_nova_backup_1444f5d0-3a87-40bc-a7a7-f3c672768b6a-00007
volume_3e388ae0-2e20-42a2-80da-3f9f366cbaed/20140402193029/az_nova_backup_1444f5d0-3a87-40bc-a7a7-f3c672768b6a-00008
volume_3e388ae0-2e20-42a2-80da-3f9f366cbaed/20140402193029/az_nova_backup_1444f5d0-3a87-40bc-a7a7-f3c672768b6a-00009
volume_3e388ae0-2e20-42a2-80da-3f9f366cbaed/20140402193029/az_nova_backup_1444f5d0-3a87-40bc-a7a7-f3c672768b6a-00010
volume_3e388ae0-2e20-42a2-80da-3f9f366cbaed/20140402193029/az_nova_backup_1444f5d0-3a87-40bc-a7a7-f3c672768b6a-00011
volume_3e388ae0-2e20-42a2-80da-3f9f366cbaed/20140402193029/az_nova_backup_1444f5d0-3a87-40bc-a7a7-f3c672768b6a-00012
volume_3e388ae0-2e20-42a2-80da-3f9f366cbaed/20140402193029/az_nova_backup_1444f5d0-3a87-40bc-a7a7-f3c672768b6a-00013
volume_3e388ae0-2e20-42a2-80da-3f9f366cbaed/20140402193029/az_nova_backup_1444f5d0-3a87-40bc-a7a7-f3c672768b6a-00014
volume_3e388ae0-2e20-42a2-80da-3f9f366cbaed/20140402193029/az_nova_backup_1444f5d0-3a87-40bc-a7a7-f3c672768b6a-00015
volume_3e388ae0-2e20-42a2-80da-3f9f366cbaed/20140402193029/az_nova_backup_1444f5d0-3a87-40bc-a7a7-f3c672768b6a-00016
volume_3e388ae0-2e20-42a2-80da-3f9f366cbaed/20140402193029/az_nova_backup_1444f5d0-3a87-40bc-a7a7-f3c672768b6a-00017
volume_3e388ae0-2e20-42a2-80da-3f9f366cbaed/20140402193029/az_nova_backup_1444f5d0-3a87-40bc-a7a7-f3c672768b6a-00018
volume_3e388ae0-2e20-42a2-80da-3f9f366cbaed/20140402193029/az_nova_backup_1444f5d0-3a87-40bc-a7a7-f3c672768b6a-00019
volume_3e388ae0-2e20-42a2-80da-3f9f366cbaed/20140402193029/az_nova_backup_1444f5d0-3a87-40bc-a7a7-f3c672768b6a-00020
volume_3e388ae0-2e20-42a2-80da-3f9f366cbaed/20140402193029/az_nova_backup_1444f5d0-3a87-40bc-a7a7-f3c672768b6a-00021
volume_3e388ae0-2e20-42a2-80da-3f9f366cbaed/20140402193029/az_nova_backup_1444f5d0-3a87-40bc-a7a7-f3c672768b6a_metadata

    $ reboot
    $ . /etc/nova/openrc && swift list cinder-backups

volume_3e388ae0-2e20-42a2-80da-3f9f366cbaed/20140402193029/az_nova_backup_1444f5d0-3a87-40bc-a7a7-f3c672768b6a-00001
volume_3e388ae0-2e20-42a2-80da-3f9f366cbaed/20140402193029/az_nova_backup_1444f5d0-3a87-40bc-a7a7-f3c672768b6a-00002
volume_3e388ae0-2e20-42a2-80da-3f9f366cbaed/20140402193029/az_nova_backup_1444f5d0-3a87-40bc-a7a7-f3c672768b6a-00003
volume_3e388ae0-2e20-42a2-80da-3f9f366cbaed/20140402193029/az_nova_backup_1444f5d0-3a87-40bc-a7a7-f3c672768b6a-00004
volume_3e388ae0-2e20-42a2-80da-3f9f366cbaed/20140402193029/az_nova_backup_1444f5d0-3a87-40bc-a7a7-f3c672768b6a-00005
volume_3e388ae0-2e20-42a2-80da-3f9f366cbaed/20140402193029/az_nova_backup_1444f5d0-3a87-40bc-a7a7-f3c672768b6a-00006
volume_3e388ae0-2e20-42a2-80da-3f9f366cbaed/20140402193029/az_nova_backup_1444f5d0-3a87-40bc-a7a7-f3c672768b6a-00007
volume_3e388ae0-2e20-42a2-80da-3f9f366cbaed/20140402193029/az_nova_backup_1444f5d0-3a87-40bc-a7a7-f3c672768b6a-00008
volume_3e388ae0-2e20-42a2-80da-3f9f366cbaed/20140402193029/az_nova_backup_1444f5d0-3a87-40bc-a7a7-f3c672768b6a-00009
volume_3e388ae0-2e20-42a2-80da-3f9f366cbaed/20140402193029/az_nova_backup_1444f5d0-3a87-40bc-a7a7-f3c672768b6a-00010
volume_3e388ae0-2e20-42a2-80da-3f9f366cbaed/20140402193029/az_nova_backup_1444f5d0-3a87-40bc-a7a7-f3c672768b6a-00011
volume_3e388ae0-2e20-42a2-80da-3f9f366cbaed/20140402193029/az_nova_backup_1444f5d0-3a87-40bc-a7a7-f3c672768b6a-00012
volume_3e388ae0-2e20-42a2-80da-3f9f366cbaed/20140402193029/az_nova_backup_1444f5d0-3a87-40bc-a7a7-f3c672768b6a-00013
volume_3e388ae0-2e20-42a2-80da-3f9f366cbaed/20140402193029/az_nova_backup_1444f5d0-3a87-40bc-a7a7-f3c672768b6a-00014
volume_3e388ae0-2e20-42a2-80da-3f9f366cbaed/20140402193029/az_nova_backup_1444f5d0-3a87-40bc-a7a7-f3c672768b6a-00015
volume_3e388ae0-2e20-42a2-80da-3f9f366cbaed/20140402193029/az_nova_backup_1444f5d0-3a87-40bc-a7a7-f3c672768b6a-00016
volume_3e388ae0-2e20-42a2-80da-3f9f366cbaed/20140402193029/az_nova_backup_1444f5d0-3a87-40bc-a7a7-f3c672768b6a-00017
volume_3e388ae0-2e20-42a2-80da-3f9f366cbaed/20140402193029/az_nova_backup_1444f5d0-3a87-40bc-a7a7-f3c672768b6a-00018
volume_3e388ae0-2e20-42a2-80da-3f9f366cbaed/20140402193029/az_nova_backup_1444f5d0-3a87-40bc-a7a7-f3c672768b6a-00019
volume_3e388ae0-2e20-42a2-80da-3f9f366cbaed/20140402193029/az_nova_backup_1444f5d0-3a87-40bc-a7a7-f3c672768b6a-00020
volume_3e388ae0-2e20-42a2-80da-3f9f366cbaed/20140402193029/az_nova_backup_1444f5d0-3a87-40bc-a7a7-f3c672768b6a-00021
volume_3e388ae0-2e20-42a2-80da-3f9f366cbaed/20140402193029/az_nova_backup_1444f5d0-3a87-40bc-a7a7-f3c672768b6a_metadata

    $ cinder backup-delete 1444f5d0-3a87-40bc-a7a7-f3c672768b6a
    $ swift list cinder-backups

(Should be empty)


Swift Built-In Unit Tests
=========================

This section describes how to run Swift and Swift client built-in unit
tests which are located at:

    /usr/lib64/python2.7/site-packages/swift/test
    /usr/lib64/python2.7/site-packages/swiftclient/tests

with nosetests test-runner.  Please make sure that the test accounts
setting in /etc/swift/test.conf reflects the keystone user accounts
setting.

To run swift built-in unit test with nosetests:

    $ To accommodate the small space of loop dev,
      modify /etc/swift/swift.conf to have "max_file_size = 5242880"
    $ /etc/init.d/swift restart
    $ cd /usr/lib64/python2.7/site-packages/swift
    $ nosetests -v test

Ran 1633 tests in 272.930s

FAILED (errors=5, failures=4)

To run swiftclient built-in unit test with nosetests:

    $ cd /usr/lib64/python2.7/site-packages/swiftclient
    $ nosetests -v tests

Ran 108 tests in 2.277s

FAILED (failures=1)


References
==========

* http://docs.openstack.org/developer/swift/deployment_guide.html
* http://docs.openstack.org/grizzly/openstack-compute/install/yum/content/ch_installing-openstack-object-storage.html
* https://swiftstack.com/openstack-swift/architecture/
