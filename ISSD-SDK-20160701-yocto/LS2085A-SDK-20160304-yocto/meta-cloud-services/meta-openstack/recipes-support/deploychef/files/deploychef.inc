#These variables are not defined in the services
#We define them here with the option for them to
#be overwritten in the future
#Nova package name, see nova recipe
SERVICE_USER ?= "nova"
#Swift test.conf
ADMIN_USER ?= "admin"
#Neutron neutron.conf
DEMO_USER ?= "demo"
DEMO_PASSWORD ?= "password"
#Use in ceilometer, openrc (nova& keystone)
OS_USERNAME ?= "admin"
OS_TENANT_NAME ?= "admin"
OS_PASSWORD ?= "password"
POSTINSTS_DIR ?= "rpm-postinsts"

