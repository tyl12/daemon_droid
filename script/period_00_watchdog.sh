. $ENV_DEEPLEARN_SCRIPT_DIR/utils_ro.sh
echo "=====>$0 begin @ `get_time`"
TAG="$0 : "

export PROCESS_PATH="/data/local/tmp"
export PROCESS_NAME="client_droid"
LOCKFILE=$PROCESS_PATH/lockfile
touch $LOCKFILE

cd $PROCESS_PATH

function check_status()
{
    WATCHDOG_TIME=$PROCESS_PATH/watchdog_file
    nowdate_sec=`date "+%s"`
    nowdate_hour=`date "+%H"`

    echo "start to check maintain reboot"
    if [ ! -f ${WATCHDOG_TIME} ]; then
        echo "NO watchdog file: $WATCHDOG_TIME, create first one"
        echo "watchdog@`get_time`@$nowdate_sec" > $WATCHDOG_TIME
        return 0
    fi
    echo "Found existing watchdog file: $WATCHDOG_TIME, contents:"
    cat $WATCHDOG_TIME

    lastdate_sec=`tail -n 1 $WATCHDOG_TIME| cut -d '@' -f 3`
    if [ "$lastdate_sec" == "" ]; then
        echo "invalid $WATCHDOG_TIME. $lastdate_sec"
        rm -rf $WATCHDOG_TIME
        return 0
    fi

    duration=$(($nowdate_sec-$lastdate_sec))
    if [ "$?" != "0" ]; then
        echo "invalid $WATCHDOG_TIME. $lastdate_sec"
        ##TODO: rm -rf $WATCHDOG_TIME
        return 0
    fi

    CHECK_LIMIT=$((2*60)) ##NOTES: 2min
    echo "now_sec: $nowdate_sec , last_sec: $lastdate_sec , duration_sec: $duration check_limit=$CHECK_LIMIT"

    if [ $duration -gt $CHECK_LIMIT ]; then
        echo "duration larger than check limit, check failed."
        return 1
    fi
    echo "check pass"
    return 0
}

function lock_file()
{
    exec 9>$LOCKFILE
    flock -n 9 #wait 10s 设置锁失败时,返回1;设

    if [ "$?" != "0" ]; then
        echo "lock $LOCKFILE failed"
        return 1
    fi
    echo "lock $LOCKFILE success"
    return 0
}
function unlock_file()
{
    flock -u 9
    exec 9>&-
    return 0
}


check_cnt=0

while true; do
    pinfo=`ps|grep -i $PROCESS_NAME`
    pinfo=`echo $pinifo`
    if [ "$pinfo" == "" ]; then
        echo "$PROCESS_NAME not launch, skip monitor this time"
        break;#next thread cycle
    fi
    echo "client_droid alive, check status"

    echo "$PROCESS_NAME exist, monitor status"
    check_status
    if [ "$?" != "0" ];then
        echo "check status failed, check_cnt=$check_cnt"
        check_cnt=$(($check_cnt+1))
        if [ "$check_cnt" -gt "10" ]; then
            echo "max check status. check_cnt=$check_cnt, reboot after 5 min..."
            echo "!!!!!!!!!!!!"
            echo "!!!!!!!!!!!!"

            sleep 300
            echo "max check status. check_cnt=$check_cnt, reboot now"
            echo "!!!!!!!!!!!!"
            echo "!!!!!!!!!!!!"
            sync

            reboot
        fi

        sleep 60
        continue #continus directly, to remember previous check_cnt
    else
        echo "check status success, reset check_cnt"
        check_cnt=0
        break;#next thread cycle
    fi
done
sleep 60

echo "=====>$0 end @ `get_time`"
