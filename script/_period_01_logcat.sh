. $ENV_XIAOMENG_SCRIPT_DIR/utils_ro.sh
echo "=====>$0 begin @ `get_time`"
TAG="$0 : "

LOGCAT_PATH="/sdcard/iceLocker/Data/log"
normal_name_prefix="${ENV_XIAOMENG_DEVICEID}_`get_time`"
log_file="${LOGCAT_PATH}/.${normal_name_prefix}.log"
mkdir -p $LOGCAT_PATH

#process all existing .xxx.log files
#leading "." & tailing ".log"
files_remain="`ls -A $LOGCAT_PATH | grep '^\.'|grep 'log$'|paste -sd ' ' -`"
echo "logcat files need to be renamed for upload:"
echo "$files_remain"

if [ "$files_remain" != "" ]; then
    #for old_name in $files_remain; do
    for old_name in `ls -A $LOGCAT_PATH | grep '^\.'|grep 'log$'`; do
        #remove leading "."
        new_name=`echo ${old_name#*.}`
        echo "rename file: $old_name -> $new_name"
        mv $LOGCAT_PATH/$old_name  $LOGCAT_PATH/$new_name
    done
fi

#start to save the new log files
echo "-------------------------------------start of logcat--------------------------------">> $log_file
timeout 1h logcat -v threadtime >> $log_file
logcat -c
sync $log_file

#echo "-------------------------------------start of dmesg--------------------------------">> $log_file
#dmesg -c >> $log_file
#sync $log_file

#generate specific dmesg logfile
dmesg_file="${LOGCAT_PATH}/.${normal_name_prefix}_dmesg.log"
dmesg -c > $dmesg_file
sync $dmesg_file

#echo "-------------------------------------start of tombstones--------------------------------">> $log_file
TOMBSTONE_PATH=/data/tombstones

files_remain="`ls -A $TOMBSTONE_PATH | grep '^tombstone'| paste -sd ' ' -`"
echo "tombstone files need to be renamed for upload:"
echo "$files_remain"

if [ "$files_remain" != "" ]; then
    #for old_name in $files_remain; do
    for old_name in `ls -A $TOMBSTONE_PATH | grep '^tombstone'`; do

        new_name=`echo ${normal_name_prefix}_${old_name}.log`
        echo "rename file: $old_name -> $new_name"
        mv $TOMBSTONE_PATH/$old_name  $LOGCAT_PATH/$new_name
    done
fi


#mv $log_file $LOGCAT_PATH/${normal_name_prefix}.log

echo "=====>$0 end @ `get_time`"
