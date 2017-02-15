#! /bin/bash

cinder type-create lvm_iscsi
cinder type-key lvm_iscsi set volume_backend_name=LVM_iSCSI
cinder type-create nfs
cinder type-key nfs set volume_backend_name=Generic_NFS
cinder type-create glusterfs
cinder type-key glusterfs set volume_backend_name=GlusterFS
cinder type-create cephrbd
cinder type-key cephrbd set volume_backend_name=RBD_CEPH
