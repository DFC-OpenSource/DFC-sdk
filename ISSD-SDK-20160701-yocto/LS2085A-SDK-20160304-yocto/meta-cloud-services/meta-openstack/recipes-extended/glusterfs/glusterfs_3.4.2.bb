#
# Copyright (C) 2013 Wind River Systems, Inc.
#

PR = "r0"

SRC_URI = "http://download.gluster.org/pub/gluster/glusterfs/3.4/${PV}/${BPN}-${PV}.tar.gz"

SRC_URI[md5sum] = "7c05304a9aca3c85ff27458461783623"
SRC_URI[sha256sum] = "4fcd42b13b60a67587de98e60ff679803433bbb0c11aa2b40c4135e2358cedef"

require glusterfs.inc
