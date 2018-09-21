. $ENV_XIAOMENG_SCRIPT_DIR/utils.sh
echo "=====>$0 begin @ `get_time`"
TAG="$0 : "

PAUSE_DAEMON_FILE="/sdcard/iceLocker/debug/disable_daemon"
if [ -f $PAUSE_DAEMON_FILE ]; then
    while true; do
        echo "${TAG}Found $PAUSE_DAEMON_FILE. pause daemon"
        sleep 60;
    done
fi

DISABLE_REBOOT_FILE="/sdcard/iceLocker/debug/disable_reboot"
if [ -f $DISABLE_REBOOT_FILE ]; then
    echo "${TAG}Found $DISABLE_REBOOT_FILE. daemon will not do reboot"
fi

##debug
echo ${TAG}$AUTOSSH_PATH
echo ${TAG}$AUTOSSH_LOGFILE

echo "=====>$0 end @ `get_time`"

