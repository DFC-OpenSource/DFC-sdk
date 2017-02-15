# This bbclass provides basic functionality for user/group settings.
# This bbclass is intended to be inherited by useradd.bbclass and
# extrausers.bbclass.

# The following functions basically have similar logic.
# *) Perform necessary checks before invoking the actual command
# *) Invoke the actual command, make retries if necessary
# *) Error out if an error occurs.

# Note that before invoking these functions, make sure the global variable
# PSEUDO is set up correctly.

perform_groupadd () {
	local rootdir="$1"
	local opts="$2"
	local retries="$3"
	bbnote "Performing groupadd with [$opts] and $retries times of retry"
	local groupname=`echo "$opts" | awk '{ print $NF }'`
	local group_exists="`grep "^$groupname:" $rootdir/etc/group || true`"
	if test "x$group_exists" = "x"; then
		local count=0
		while true; do
			eval $PSEUDO groupadd $opts || true
			group_exists="`grep "^$groupname:" $rootdir/etc/group || true`"
			if test "x$group_exists" = "x"; then
				bbwarn "groupadd command did not succeed. Retrying..."
			else
				break
			fi
			count=`expr $count + 1`
			if test $count = $retries; then
				bbfatal "Tried running groupadd command $retries times without success, giving up"
			fi
                        sleep $count
		done
	else
		bbwarn "group $groupname already exists, not re-creating it"
	fi
}

perform_useradd () {
	local rootdir="$1"
	local opts="$2"
	local retries="$3"
	bbnote "Performing useradd with [$opts] and $retries times of retry"
	local username=`echo "$opts" | awk '{ print $NF }'`
	local user_exists="`grep "^$username:" $rootdir/etc/passwd || true`"
	if test "x$user_exists" = "x"; then
	       local count=0
	       while true; do
		       eval $PSEUDO useradd $opts || true
		       user_exists="`grep "^$username:" $rootdir/etc/passwd || true`"
		       if test "x$user_exists" = "x"; then
			       bbwarn "useradd command did not succeed. Retrying..."
		       else
			       break
		       fi
		       count=`expr $count + 1`
		       if test $count = $retries; then
				bbfatal "Tried running useradd command $retries times without success, giving up"
		       fi
		       sleep $count
	       done
	else
		bbwarn "user $username already exists, not re-creating it"
	fi
}

perform_groupmems () {
	local rootdir="$1"
	local opts="$2"
	local retries="$3"
	bbnote "Performing groupmems with [$opts] and $retries times of retry"
	local groupname=`echo "$opts" | awk '{ for (i = 1; i < NF; i++) if ($i == "-g" || $i == "--group") print $(i+1) }'`
	local username=`echo "$opts" | awk '{ for (i = 1; i < NF; i++) if ($i == "-a" || $i == "--add") print $(i+1) }'`
	bbnote "Running groupmems command with group $groupname and user $username"
	# groupmems fails if /etc/gshadow does not exist
	local gshadow=""
	if [ -f $rootdir${sysconfdir}/gshadow ]; then
		gshadow="yes"
	else
		gshadow="no"
		touch $rootdir${sysconfdir}/gshadow
	fi
	local mem_exists="`grep "^$groupname:[^:]*:[^:]*:\([^,]*,\)*$username\(,[^,]*\)*" $rootdir/etc/group || true`"
	if test "x$mem_exists" = "x"; then
		local count=0
		while true; do
			eval $PSEUDO groupmems $opts || true
			mem_exists="`grep "^$groupname:[^:]*:[^:]*:\([^,]*,\)*$username\(,[^,]*\)*" $rootdir/etc/group || true`"
			if test "x$mem_exists" = "x"; then
				bbwarn "groupmems command did not succeed. Retrying..."
			else
				break
			fi
			count=`expr $count + 1`
			if test $count = $retries; then
				if test "x$gshadow" = "xno"; then
					rm -f $rootdir${sysconfdir}/gshadow
					rm -f $rootdir${sysconfdir}/gshadow-
				fi
				bbfatal "Tried running groupmems command $retries times without success, giving up"
			fi
			sleep $count
		done
	else
		bbwarn "group $groupname already contains $username, not re-adding it"
	fi
	if test "x$gshadow" = "xno"; then
		rm -f $rootdir${sysconfdir}/gshadow
		rm -f $rootdir${sysconfdir}/gshadow-
	fi
}

perform_groupdel () {
	local rootdir="$1"
	local opts="$2"
	local retries="$3"
	bbnote "Performing groupdel with [$opts] and $retries times of retry"
	local groupname=`echo "$opts" | awk '{ print $NF }'`
	local group_exists="`grep "^$groupname:" $rootdir/etc/group || true`"
	if test "x$group_exists" != "x"; then
		local count=0
		while true; do
			eval $PSEUDO groupdel $opts || true
			group_exists="`grep "^$groupname:" $rootdir/etc/group || true`"
			if test "x$group_exists" != "x"; then
				bbwarn "groupdel command did not succeed. Retrying..."
			else
				break
			fi
			count=`expr $count + 1`
			if test $count = $retries; then
				bbfatal "Tried running groupdel command $retries times without success, giving up"
			fi
			sleep $count
		done
	else
		bbwarn "group $groupname doesn't exist, not removing it"
	fi
}

perform_userdel () {
	local rootdir="$1"
	local opts="$2"
	local retries="$3"
	bbnote "Performing userdel with [$opts] and $retries times of retry"
	local username=`echo "$opts" | awk '{ print $NF }'`
	local user_exists="`grep "^$username:" $rootdir/etc/passwd || true`"
	if test "x$user_exists" != "x"; then
	       local count=0
	       while true; do
		       eval $PSEUDO userdel $opts || true
		       user_exists="`grep "^$username:" $rootdir/etc/passwd || true`"
		       if test "x$user_exists" != "x"; then
			       bbwarn "userdel command did not succeed. Retrying..."
		       else
			       break
		       fi
		       count=`expr $count + 1`
		       if test $count = $retries; then
				bbfatal "Tried running userdel command $retries times without success, giving up"
		       fi
		       sleep $count
	       done
	else
		bbwarn "user $username doesn't exist, not removing it"
	fi
}

perform_groupmod () {
	# Other than the return value of groupmod, there's no simple way to judge whether the command
	# succeeds, so we disable -e option temporarily
	set +e
	local rootdir="$1"
	local opts="$2"
	local retries="$3"
	bbnote "Performing groupmod with [$opts] and $retries times of retry"
	local groupname=`echo "$opts" | awk '{ print $NF }'`
	local group_exists="`grep "^$groupname:" $rootdir/etc/group || true`"
	if test "x$group_exists" != "x"; then
		local count=0
		while true; do
			eval $PSEUDO groupmod $opts
			if test $? != 0; then
				bbwarn "groupmod command did not succeed. Retrying..."
			else
				break
			fi
			count=`expr $count + 1`
			if test $count = $retries; then
				bbfatal "Tried running groupmod command $retries times without success, giving up"
			fi
			sleep $count
		done
	else
		bbwarn "group $groupname doesn't exist, unable to modify it"
	fi
	set -e
}

perform_usermod () {
	# Same reason with groupmod, temporarily disable -e option
	set +e
	local rootdir="$1"
	local opts="$2"
	local retries="$3"
	bbnote "Performing usermod with [$opts] and $retries times of retry"
	local username=`echo "$opts" | awk '{ print $NF }'`
	local user_exists="`grep "^$username:" $rootdir/etc/passwd || true`"
	if test "x$user_exists" != "x"; then
	       local count=0
	       while true; do
		       eval $PSEUDO usermod $opts
		       if test $? != 0; then
			       bbwarn "usermod command did not succeed. Retrying..."
		       else
			       break
		       fi
		       count=`expr $count + 1`
		       if test $count = $retries; then
				bbfatal "Tried running usermod command $retries times without success, giving up"
		       fi
		       sleep $count
	       done
	else
		bbwarn "user $username doesn't exist, unable to modify it"
	fi
	set -e
}
