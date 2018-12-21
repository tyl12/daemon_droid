. $ENV_DEEPLEARN_SCRIPT_DIR/utils_ro.sh
echo "=====>$0 begin @ `get_time`"
TAG="$0 : "

LOGCAT_PATH="/data/local/tmp/log"
normal_name_prefix="logfile_`get_time`"
log_file="${LOGCAT_PATH}/${normal_name_prefix}.log"

if [ -d $LOGCAT_PATH ]; then
    size=`du -m $LOGCAT_PATH| grep -Eo "^[0-9]+"`
    echo "Found log size ${size} M"
    if [ "$size" -gt "200" ]; then
        echo "clean log dir"
        rm -rf $LOGCAT_PATH
    fi
fi
mkdir -p $LOGCAT_PATH

#start to save the new log files
echo "-------------------------------------start of logcat--------------------------------">> $log_file
timeout 1h logcat -v threadtime >> $log_file
logcat -c
sync $log_file

echo "=====>$0 end @ `get_time`"
