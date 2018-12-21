. $ENV_DEEPLEARN_SCRIPT_DIR/utils_ro.sh
echo "=====>$0 begin @ `get_time`"
TAG="$0 : "

export PROCESS_PATH="/data/local/tmp"
export PROCESS_NAME="deeplearn_client"


#----------------------------------------
DAEMON_LAUNCH_TIMESTAMP="$PROCESS_PATH/daemon_launch_timestamp"

RESET_CTX_LIMIT=20
echo "update $DAEMON_LAUNCH_TIMESTAMP"
echo "launch daemon@`get_time`" >> $DAEMON_LAUNCH_TIMESTAMP
tmpfile=$PROCESS_PATH/.daemon_launch_timestamp
tail -n $RESET_CTX_LIMIT $DAEMON_LAUNCH_TIMESTAMP > $tmpfile
mv $tmpfile $DAEMON_LAUNCH_TIMESTAMP

#----------------------------------------


cnt=0
while true; do
    cd  $PROCESS_PATH
    if [ ! -f ./$PROCESS_NAME ]; then
        echo "ERROR: $PROCESS_PATH/$PROCESS_NAME not found"
        sleep 10
        continue
    fi

    echo "launch cnt= $cnt"
    chmod 777 ./$PROCESS_NAME
    ./$PROCESS_NAME

    if [ "$?" != "0" ]; then
        echo "ERROR: found process die. $cnt times"
        cnt=$(($cnt+1))
    fi

    if [ $cnt -gt "10" ]; then
        echo "ERROR: max launch time, reboot system after 5min"
        echo "!!!!!!!!!!!!"
        echo "!!!!!!!!!!!!"

        sleep 300
        echo "ERROR: max launch time, reboot system now"
        echo "!!!!!!!!!!!!"
        echo "!!!!!!!!!!!!"
        sync

        reboot
    fi
    sleep 10
done

echo "=====>$0 end @ `get_time`"
