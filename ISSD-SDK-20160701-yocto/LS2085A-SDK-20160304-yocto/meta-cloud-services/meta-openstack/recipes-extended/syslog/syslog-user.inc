#
# Copyright (C) 2014 Wind River Systems, Inc.
#

inherit useradd

USERADD_PACKAGES = "${PN}"
GROUPADD_PARAM_${PN} = "--system syslog"
USERADD_PARAM_${PN}  = "--system --home /var/log -g syslog syslog"

