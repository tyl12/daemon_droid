
get_cpu(){
    s=`cat /proc/cpuinfo | grep -i intel`
    if [ "$s" != "" ]; then
        echo "x86"
    else
        echo "arm"
    fi
}

get_time(){
    TIME_POSTFIX="$(date +%Y%m%d_%H%M_%S)"
    echo $TIME_POSTFIX
}

launch_ssh(){
    #cpu_info="`get_cpu`"
    #echo "$cpu_info"
    #if [ "$cpu_info" == "x86" ]; then
    #    res="`ps aux | grep sshd`"
    #else
    #    res="`ps | grep sshd`"
    #fi

    res="`pgrep sshd|paste -sd ',' -`"
    if [ "$res" == "" ]; then
        echo "sshd not running. launch immediately"
        /system/bin/sshd -f /system/etc/ssh/sshd_config
    else
        echo "sshd running well"
    fi

    res="`pgrep sshd|paste -sd ',' -`"
    if [ "$res" == "" ]; then
        echo "autossh not running. launch immediately"
        autossh -f -M  $ENV_XIAOMENG_RSSH_MONITOR  -NR $ENV_XIAOMENG_RSSH_PORT:localhost:22 -i '/data/ssh/ssh_host_rsa_key' -o 'StrictHostKeyChecking=no' -o 'ServerAliveInterval=60' -o 'ServerAliveCountMax=3' $ENV_XIAOMENG_RSSH_USER@$ENV_XIAOMENG_RSSH_SVRIP -p $ENV_XIAOMENG_RSSH_SVRPORT
    else
        echo "autossh running well"
    fi
}

export LOG_TAG="#####->"
export get_time
export launch_ssh

