#
# Copyright (C) 2013 Wind River Systems, Inc.
#

require irqbalance.inc

PR = "r0"

SRC_URI[md5sum] = "b73b1a5a9e1c3c428ae39024c711e41e"
SRC_URI[sha256sum] = "95ac79992e6de501f613c781b0fc8aa17a4aaf6a3d865bb6e15ac6a46c6ab1fd"

SRC_URI = "http://irqbalance.googlecode.com/files/irqbalance-${PV}.tar.gz \
           file://add-initscript.patch \
           file://irqbalance-Add-status-and-reload-commands.patch \
          "
