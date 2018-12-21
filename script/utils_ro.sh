#******NEVER TOUCH THIS! BEGIN********************************
export ANDROID_DATA=/data
export LD_LIBRARY_PATH=.
export ANDROID_ROOT=/system

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

export LOG_TAG="#####->"
export get_time
export launch_ssh

#******NEVER TOUCH THIS! END********************************
