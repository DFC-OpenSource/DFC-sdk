#
# all-in-one hosts file
#
# The aio rootfs combines the functionality of the compute and 
# the controller in one node.
#
# The aio rootfs adds another hostname aio, since we need
# ths hostname compute and controller for the system's functionality,
# we defined compute and controller in this file as well. 
#
COMPUTE_IP ?= "${CONTROLLER_IP}"
COMPUTE_HOST ?= "compute"
CONTROLLER_IP ?= "128.224.149.173"
CONTROLLER_HOST ?= "controller"
MY_IP ?= "${CONTROLLER_IP}"
MY_HOST ?= "${CONTROLLER_HOST}"
DB_DATADIR ?= "/etc/postgresql"
