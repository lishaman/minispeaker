#!/bin/sh

cgroup_init()
{
    if [ ! -d /mnt/cgroup/app ]; then
        /bin/mount -t cgroup -o memory memory /mnt/cgroup/
        /bin/mkdir /mnt/cgroup/app
        /bin/echo 3 > /mnt/cgroup/app/memory.move_charge_at_immigrate
        /bin/echo 1 > /mnt/cgroup/app/memory.use_hierarchy
    fi
}

cgroup_attach()
{
    if [ $# -eq 2 ]; then
        NAME=$1
    else
        NAME=$2
    fi
    PID=$2

    cgroup_init

    /bin/echo "cgroup attach $NAME($PID)"

    if [ -d /mnt/cgroup/app/$NAME ]; then
        /bin/rm -rf /mnt/cgroup/app/$NAME 2>/dev/null
    fi

    /bin/mkdir /mnt/cgroup/app/$NAME
    if [ $? != 0 ]; then
        exit
    fi
    /bin/echo 1 > /mnt/cgroup/app/$NAME/memory.move_charge_at_immigrate
    /bin/echo 1 > /mnt/cgroup/app/$NAME/memory.cache_charge_on_parent
    /bin/echo $PID > /mnt/cgroup/app/cgroup.procs
    /bin/echo $PID > /mnt/cgroup/app/$1/cgroup.procs
}

cgroup_attach_comm()
{
    PS=`/bin/ps l`
    #/bin/echo "$PS" | /bin/grep -E ":[0-9]{2}( $1 | $1$)"
    PID=`/bin/echo "$PS" | /bin/grep -E ":[0-9]{2}( $1 | $1$| .*/$1)" | /usr/bin/awk '{print $3}'`
    #/bin/echo $1" pid: "$PID

    if [ "$PID"x != x ]; then
        cgroup_attach $1 $PID
    fi
}

cgroup_attach_pid()
{
    cgroup_attach $1 $1
}

if [ $# -eq 2 ]; then
    if [ $1 = "-c" ]; then
        cgroup_attach_comm $2
    elif [ $1 = "-p" ]; then
        cgroup_attach_pid $2
    fi
elif [ $# -eq 1 ]; then
    if [ $1 = "mozart" ]; then
       cgroup_attach_comm mozart
       cgroup_attach_comm network_manager
       cgroup_attach_comm player
       cgroup_attach_comm event_manager
       cgroup_attach_comm shairport
       cgroup_attach_comm mplayer
       cgroup_attach_comm lapsule
       cgroup_attach_comm bsa_server_mips
       cgroup_attach_comm wpa_supplicant
       cgroup_attach_comm app_service
       cgroup_attach_comm httpd
       cgroup_attach_comm adbd
       cgroup_attach_comm ntpd
    fi
else
    /bin/echo "Usage: $0 [OPTIONS] [FILE]"
    /bin/echo ""
    /bin/echo "Options:"
    /bin/echo "    -c <command>"
    /bin/echo "    -p <pid>"
    /bin/echo "    mozart"
fi
