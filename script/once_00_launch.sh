. $ENV_DEEPLEARN_SCRIPT_DIR/utils_ro.sh
echo "=====>$0 begin @ `get_time`"
TAG="$0 : "

export ANDROID_DATA=/data
export LD_LIBRARY_PATH=.
export ANDROID_ROOT=/system

cnt=0
while true; do
    cd /data/local/tmp/
    if [ ! -f ./client_droid ]; then
        echo "ERROR: /data/local/tmp/client_droid not found"
        sleep 10
        continue
    fi

    echo "launch cnt= $cnt"
    chmod 777 ./client_droid
    ./client_droid

    if [ "$?" != "0" ]; then
        echo "ERROR: found process die"
        cnt=$(($cnt+1))
    fi

    if [ $cnt -gt "10" ]; then
        echo "ERROR: max launch time, reboot system after 10min"
        sleep 600
        echo "ERROR: max launch time, reboot system now"
        reboot
    fi
    sleep 10
done

echo "=====>$0 end @ `get_time`"
