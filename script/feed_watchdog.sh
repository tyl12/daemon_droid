#******NEVER TOUCH THIS! BEGIN********************************

export ANDROID_DATA=/data
export LD_LIBRARY_PATH=.
export ANDROID_ROOT=/system

get_time(){
    TIME_POSTFIX="$(date +%Y%m%d_%H%M_%S)"
    echo $TIME_POSTFIX
}

export PROCESS_PATH="/data/local/tmp"
export WATCHDOG_TIME=$PROCESS_PATH/watchdog_file

nowdate_sec=`date "+%s"`
nowdate_hour=`date "+%H"`
echo "watchdog@`get_time`@$nowdate_sec" > $WATCHDOG_TIME

echo "feed watchdog"

#******NEVER TOUCH THIS! END********************************
