#!/bin/sh

HOME=/root
PATH=/sbin:/bin:/usr/sbin:/usr/bin
PS1="linaro-test [rc=$(echo \$?)]# "
export HOME PS1 PATH

early_setup() {
    mkdir -p /proc /sys /tmp /run
    mount -t proc proc /proc
    mount -t sysfs sysfs /sys
    mount -t devtmpfs none /dev

    ln -s /run /var/run

    chmod 0666 /dev/tty*
    chown root:tty /dev/tty*
}

read_args() {
    [ -z "$CMDLINE" ] && CMDLINE=`cat /proc/cmdline`
    for arg in $CMDLINE; do
        optarg=`expr "x$arg" : 'x[^=]*=\(.*\)'`
        case $arg in
            console=*)
                tty=${arg#console=}
                tty=${tty#/dev/}

                case $tty in
                    tty[a-zA-Z]* )
                        port=${tty%%,*}
                esac ;;
            debug) set -x ;;
        esac
    done
}

early_setup
read_args

setsid sh </dev/${port} >/dev/${port} 2>&1
