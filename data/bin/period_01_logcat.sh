. $ENV_XIAOMENG_SCRIPT_DIR/utils.sh
echo "=====>$0 begin @ `get_time`"
TAG="$0 : "


LOGCAT_PATH="/sdcard/iceLocker/Data/log/need_upload_logs"
normal_name="${ENV_XIAOMENG_DEVICEID}_`get_time`.log"
log_file="$LOGCAT_PATH/.$normal_name"

echo "-------------------------------------start of logcat--------------------------------">> $log_file
timeout 2m logcat -v threadtime >> $log_file
logcat -c
echo "-------------------------------------start of dmesg--------------------------------">> $log_file
dmesg -c >> $log_file

#mv log_file $LOGCAT_PATH/$normal_name

#leading "." & tailing ".log"
files_remain="`ls -A $LOGCAT_PATH | grep '^\.'|grep 'log$'|paste -sd ' ' -`"
echo "${TAG}files need to be renamed for upload:"
echo "${TAG}$files_remain"

if [ "$files_remain" != "" ]; then
    #for old_name in $files_remain; do
    for old_name in `ls -A $LOGCAT_PATH | grep '^\.'|grep 'log$'`; do
        #remove leading "."
        new_name=`echo ${old_name#*.}`
        echo "${TAG}rename file: $old_name -> $new_name"
        mv $LOGCAT_PATH/$old_name  $LOGCAT_PATH/$new_name
    done
fi

echo "=====>$0 end @ `get_time`"
