#!/bin/bash

#
# Copyright (C) 2014 Wind River Systems, Inc.
#
# The identity.sh provides utilities for services to add tenant/role/users,
# and service/endpoints into keystone database
#

# Use shared secret for authentication before any user created.
export OS_SERVICE_TOKEN="password"
export OS_SERVICE_ENDPOINT="http://localhost:35357/v2.0"

declare -A PARAMS

# Shortcut function to get a newly generated ID
function get_field () {
    while read data; do
        if [ "$1" -lt 0 ]; then
            field="(\$(NF$1))"
        else
            field="\$$(($1 + 1))"
        fi
        echo "$data" | awk -F'[ \t]*\\|[ \t]*' "{print $field}"
    done
}

# Usage help
help () {
    if [ $# -eq 0 ]; then
        echo "Usage: $0 <subcommand> ..."
        echo ""
        echo "Keystone CLI wrapper to create tenant/user/role, and service/endpoint."
        echo "It uses the default tenant, user and password from environment variables"
        echo "(OS_TENANT_NAME, OS_USERNAME, OS_PASSWORD) to authenticate with keystone."
        echo ""
        echo "Positional arguments:"
        echo "  <subcommand>"
        echo "    user-create"
        echo "    service-create"
        echo ""
        echo "See \"identity.sh help COMMAND\" for help on a specific command."
        exit 0
    fi

    case "$2" in
        service-create)
            echo "Usage: $0 $2 [--name=<name>] [--type=<type>] [--description=<description>] [--region=<region>] [--publicurl=<public url>] [--adminurl=<admin url>] [--internalurl=<internal url>]"
            echo ""
            echo "Create service and endpoint in keystone."
            echo ""
            echo "Arguments:"
            echo "  --name=<name>"
            echo "                  The name of the service"
            echo "  --type=<type>"
            echo "                  The type of the service"
            echo "  --description=<description>"
            echo "                  Description of the service"
            echo "  --region=<region>"
            echo "                  The region of the service"
            echo "  --publicurl=<public url>"
            echo "                  Public URL of the service endpoint"
            echo "  --adminurl=<admin url>"
            echo "                  Admin URL of the service endpoint"
            echo "  --internalurl=<internal url>"
            echo "                  Internal URL of the service endpoint"
            ;;
        user-create)
            echo "Usage: $0 $2 [--name=<name>] [--pass=<password>] [--tenant=<tenant>] [--role=<role>] [--email=<email>]"
            echo ""
            echo "Arguments:"
            echo "  --name=<name>"
            echo "                  The name of the user"
            echo "  --pass=<password>"
            echo "                  The password of the user"
            echo "  --tenant=<tenant>"
            echo "                  The tenant of the user belongs to"
            echo "  --role=<role>"
            echo "                  The role of the user in the <tenant>"
            echo "  --email=<email>"
            echo "                  The email of the user"
            ;;
        *)
            echo "Usage: $0 help <subcommand> ..."
            echo ""
            exit 0
            ;;
    esac
}

# Parse the command line parameters in an map
parse_param () {
    while [ $# -ne 0 ]; do
    param=$1
    shift

    key=`echo $param | cut -d '=' -f 1`
    key=`echo $key | tr -d '[-*2]'`
    PARAMS[$key]=`echo $param | cut -d '=' -f 2`
    done
}

# Create tenant/role/user, and add user to the tenant as role
user-create () {
    # validation checking
    if [[ "$@" =~ ^--name=.*\ --pass=.*\ --tenant=.*\ --role=.*\ --email=.*$ ]]; then
        params=`echo "$@" | sed -e 's%--name=\(.*\) --pass=\(.*\) --tenant=\(.*\) --role=\(.*\) --email=\(.*\)%--name=\1|--pass=\2|--tenant=\3|--role=\4|--email=\5%g'`
    else
        help
        exit 1
    fi

    # parse the cmdline parameters
    IFS="|"
    parse_param $params
    unset IFS

    echo "Adding user in keystone ..."

    if [ "x${PARAMS["tenant"]}" != "x" ]; then
        # check if tenant exist, create it if not
        TENANT_ID=$(keystone tenant-get ${PARAMS["tenant"]} | grep " id " | get_field 2)
        if [ "x$TENANT_ID" == "x" ]; then
            echo "Creating tenant ${PARAMS["tenant"]} in keystone ..."
            TENANT_ID=$(keystone tenant-create --name=${PARAMS["tenant"]} | grep " id " | get_field 2)
        fi
        echo "Tenant list:"
        keystone tenant-list
    fi

    if [ "x${PARAMS["role"]}" != "x" ]; then
        # check if role exist, create it if not
        ROLE_ID=$(keystone role-get ${PARAMS["role"]} | grep " id " | get_field 2)
        if [ "x$ROLE_ID" == "x" ]; then
            echo "Creating role ${PARAMS["role"]} in keystone ..."
            ROLE_ID=$(keystone role-create --name=${PARAMS["role"]} | grep " id " | get_field 2)
        fi
        echo "Role list:"
        keystone role-list
    fi

    if [ "x${PARAMS["name"]}" != "x" ]; then
        # check if user exist, create it if not
        USER_ID=$(keystone user-get ${PARAMS["name"]} | grep " id " | get_field 2)
        if [ "x$USER_ID" == "x" ]; then
            echo "Creating user ${PARAMS["name"]} in keystone ..."
            USER_ID=$(keystone user-create --name=${PARAMS["name"]} --pass=${PARAMS["pass"]} --tenant-id $TENANT_ID --email=${PARAMS["email"]} | grep " id " | get_field 2)
        fi
        echo "User list:"
        keystone user-list
    fi

    if [ "x$USER_ID" != "x" ] && [ "x$TENANT_ID" != "x" ] && [ "x$ROLE_ID" != "x" ]; then
        # add the user to the tenant as role
        keystone user-role-list --user-id $USER_ID --tenant-id $TENANT_ID | grep $ROLE_ID &> /dev/null
        if [ $? -eq 1 ]; then
            echo "Adding user ${PARAMS["name"]} in tenant ${PARAMS["tenant"]} as ${PARAMS["role"]} ..."
            keystone user-role-add --tenant-id $TENANT_ID --user-id $USER_ID --role-id $ROLE_ID
        fi
    fi

    if [ "x$USER_ID" != "x" ] && [ "x$TENANT_ID" != "x" ]; then
        echo "User ${PARAMS["name"]} in Tenant ${PARAMS["tenant"]} role list:"
        keystone user-role-list --user-id $USER_ID --tenant-id $TENANT_ID
    fi
}

# Create service and its endpoint
service-create () {
    # validation checking
    if [[ "$@" =~ ^--name=.*\ --type=.*\ --description=.*\ --region=.*\ --publicurl=.*\ --adminurl=.*\ --internalurl=.*$ ]]; then
        params=`echo "$@" | sed -e 's%--name=\(.*\) --type=\(.*\) --description=\(.*\) --region=\(.*\) --publicurl=\(.*\) --adminurl=\(.*\) --internalurl=\(.*\)%--name=\1|--type=\2|--description=\3|--region=\4|--publicurl=\5|--adminurl=\6|--internalurl=\7%g'`
    else
        help
        exit 1
    fi

    # parse the cmdline parameters
    IFS=$"|"
    parse_param $params
    unset IFS

    echo "Creating service in keystone ..."

    if [ "x${PARAMS["name"]}" != "x" ]; then
        # check if service already created, create it if not
        SERVICE_ID=$(keystone service-get ${PARAMS["name"]} | grep " id " | get_field 2)
        if [ "x$SERVICE_ID" == "x" ]; then
            echo "Adding service ${PARAMS["name"]} in keystone ..."
            SERVICE_ID=$(keystone service-create --name ${PARAMS["name"]} --type ${PARAMS["type"]} --description "${PARAMS["description"]}" | grep " id " | get_field 2)
        fi
        echo "Service list:"
        keystone service-list
    fi

    if [ "x$SERVICE_ID" != "x" ]; then
        # create its endpoint
        keystone endpoint-list | grep $SERVICE_ID | grep ${PARAMS["region"]} | grep ${PARAMS["publicurl"]} | grep ${PARAMS["adminurl"]} | grep ${PARAMS["internalurl"]}
        if [ $? -eq 1 ]; then
            echo "Creating endpoint for ${PARAMS["name"]} in keystone ..."
            keystone endpoint-create --region ${PARAMS["region"]} --service-id $SERVICE_ID --publicurl ${PARAMS["publicurl"]} --adminurl ${PARAMS["adminurl"]} --internalurl ${PARAMS["internalurl"]}
        fi
        echo "Endpoints list:"
        keystone endpoint-list
    fi
}

case "$1" in
    service-create)
        shift
        service-create $@
        ;;
    user-create)
        shift
        user-create $@
        ;;
    help)
        help $@
        ;;
    *)
        help
        exit 0
        ;;
esac

exit 0
