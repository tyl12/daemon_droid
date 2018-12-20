. $ENV_XIAOMENG_SCRIPT_DIR/utils_ro.sh
echo "=====>$0 begin @ `get_time`"
TAG="$0 : "


APP_PROC_NAME="com.xiaomeng.icelocker"
FACTORY_PROC_NAME="com.xiaomeng.androidselftest"

APP_ACTIVITY="com.xiaomeng.icelocker/com.xiaomeng.iceLocker.ui.activity.MainActivity"

IceLocker_UPGRADE_DIR="/sdcard/iceLocker/IceLockerUpdatePkg"
IceLocker_UPGRADE_FLAG="/sdcard/iceLocker/IceLockerVersion"
IceLocker_WORK_PATH="/sdcard/iceLocker/IceLocker"

Template_UPGRADE_DIR="/sdcard/iceLocker/TemplateUpdatePkg"
Template_UPGRADE_FLAG="/sdcard/iceLocker/TemplateVersion"
Template_WORK_PATH="/sdcard/iceLocker/Template"

APP_RELAUNCH_FILE="/sdcard/iceLocker/Data/app_relaunch_count"

XIAOMENG_DEBUG_DIR="/sdcard/iceLocker/debug"
DISABLE_MONITOR_FILE="$XIAOMENG_DEBUG_DIR/disable_monitor"
DISABLE_REBOOT_FILE="$XIAOMENG_DEBUG_DIR/disable_reboot"

mkdir -p $IceLocker_WORK_PATH
mkdir -p $Template_WORK_PATH
mkdir -p $IceLocker_UPGRADE_DIR
mkdir -p /sdcard/iceLocker/Data
mkdir -p $XIAOMENG_DEBUG_DIR

#input:
#   $1: the dir that contains the new pkg need to upgrade
#   $2: the file that contins the new pkg name only, without the absolute path prefix
#   $3: the dir that the new pkg contents need to be copied to.
#output:
#   this function will parse $2 contents to get the name of new pkg, and search in upgrade dir $1
#       if found, echo the absolute path  $1/$2 and return 0
#       else, not echo anything and return nonzero
function get_upgrade_src()
{
    upgrade_dir="$1"
    upgrade_file="$2"
    upgrade_dest="$3"

    if [ ! -f $upgrade_file ]; then
        return 1
    fi

    #remove tailing whitespace if any
    upgrade_cont_orig="`head -n 1 $upgrade_file | tr -d '\r'`"
    upgrade_cont=`echo $upgrade_cont_orig`
    #echo "$upgrade_file contents: $upgrade_cont"
    if [ "$upgrade_cont" == "" ]; then
        return 1;
    fi

    upgrade_src="$upgrade_dir/$upgrade_cont"
    #echo "Found contents: $upgrade_cont in $upgrade_file. search for $upgrade_src"
    if [ ! -d ${upgrade_src} ]; then
        return 1;
    fi
    echo "$upgrade_src"
    return 0;
}

function copy_upgrade_src()
{
    src_dir="$1"
    dest_dir="$2"
    echo "copy $src_dir -> ${dest_dir}_tmp"
    if [ ! -d $src_dir ]; then
        echo "ERROR: $src_dir not found, return."
        return 1
    fi

    rm -rf ${dest_dir}_tmp
    cp -r $src_dir  ${dest_dir}_tmp
    ret=$?
    if [[ $ret != 0 ]]; then
        echo "ERROR: fail to copy"
        return $ret
    fi
    chmod 777 ${dest_dir}_tmp -R

    echo "rename ${dest_dir}_tmp -> ${dest_dir}"
    rm -rf ${dest_dir}
    mv ${dest_dir}_tmp ${dest_dir}
    ret=$?
    if [[ $ret != 0 ]]; then
        echo "ERROR: fail to rename"
        return $ret
    fi

    sync
}

#***************************************************************
#if factorytest app is alive,
#   kill main app.
#   sleep 10sec
#   return
#else
#   if IceLockerVersion file is available and checksum pass
#       kill main app and do upgrade
#       sleep 10s
#   if com.xiaomeng.icelocker is not installed
#       sleep 5min and return
#
#   check main app status
#   if not alive
#       launch main app
#       if success:
#           sleep 60s
#       else:
#           sleep 10s
#   else
#       sleep 60s
#
#***************************************************************
factorypid="`pidof $FACTORY_PROC_NAME`"
if [ "$factorypid" != "" ]; then
    echo "Found $FACTORY_PROC_NAME alive, kill $APP_PROC_NAME for factory test"
    mainpid="`pidof $APP_PROC_NAME`"
    if [ "$mainpid" != "" ]; then
        kill -9 $mainpid
    else
        echo "$APP_PROC_NAME is alread die, continue"
    fi
    sleep 10
    return
fi

icelocker_success="1" #false by default
template_success="1"

upgrade_dir=$IceLocker_UPGRADE_DIR
upgrade_file=$IceLocker_UPGRADE_FLAG
upgrade_dest=$IceLocker_WORK_PATH
IceLocker_UpgradeSrc=`get_upgrade_src $upgrade_dir  $upgrade_file   $upgrade_dest`
IceLocker_UpgradeSrc=`echo $IceLocker_UpgradeSrc`
echo "get_upgrade_src return IceLocker_UpgradeSrc=$IceLocker_UpgradeSrc"

upgrade_dir=$Template_UPGRADE_DIR
upgrade_file=$Template_UPGRADE_FLAG
upgrade_dest=$Template_WORK_PATH
Template_UpgradeSrc=`get_upgrade_src $upgrade_dir  $upgrade_file   $upgrade_dest`
Template_UpgradeSrc=`echo $Template_UpgradeSrc`
echo "get_upgrade_src return Template_UpgradeSrc=$Template_UpgradeSrc"

if [[ "$IceLocker_UpgradeSrc" != "" ]]; then
    echo "Found matching $IceLocker_UpgradeSrc. do update..."

    mainpid="`pidof $APP_PROC_NAME`"
    #kill main app first
    if [ "$mainpid" != "" ]; then
        echo "Found $APP_PROC_NAME alive, kill first for update"
        kill -9 $mainpid
        sleep 2
    fi

    copy_upgrade_src $IceLocker_UpgradeSrc  $IceLocker_WORK_PATH
    ret="$?"
    cp  $IceLocker_UPGRADE_FLAG  $IceLocker_WORK_PATH/

    if [ "$ret" != "0" ]; then
        echo "fail to copy $IceLocker_UpgradeSrc -> $IceLocker_WORK_PATH, exit for next retry"
        return
    fi

    #NOTES: IceLocker_WORK_PATH should use absolute path, so that apklist will contain the absolute path prefix.
    apklist="`ls $IceLocker_WORK_PATH/*.apk`"
    for apk in "$apklist" ; do
        echo "install $apk"
        pm install -r -t --user 0 ${apk}
        if [ "$?" != "0" ]; then
            echo "fail to install $apk, exit for next retry"
            sleep 2
            return
        else
            echo "success to install $apk"
            echo "" > $IceLocker_UPGRADE_FLAG
            sleep 2
        fi
    done
fi

if [[ "$Template_UpgradeSrc" != "" ]]; then
    echo "Found matching $Template_UpgradeSrc. do update..."

    mainpid="`pidof $APP_PROC_NAME`"
    #kill main app first
    if [ "$mainpid" != "" ]; then
        echo "Found $APP_PROC_NAME alive, kill first for update"
        kill -9 $mainpid
        sleep 2
    fi

    copy_upgrade_src $Template_UpgradeSrc  $Template_WORK_PATH
    ret="$?"
    cp  $Template_UPGRADE_FLAG  $Template_WORK_PATH/

    if [ "$ret" != "0" ]; then
        echo "fail to copy $Template_UpgradeSrc -> $Template_WORK_PATH, exit for next retry"
        return
    fi

    echo "success update Template!"
    echo "" > $Template_UPGRADE_FLAG

    downloadserverpid="`pidof .IceLockerDownload`"
    if [ "$downloadserverpid" != "" ]; then
        echo "Kill download server for template update"
        kill -9  $downloadserverpid
    fi

    sleep 2
fi

if [ -f $DISABLE_MONITOR_FILE ]; then
    echo "Found $DISABLE_MONITOR_FILE. daemon will not do app monitor"
    sleep 60
    return
fi

pm path $APP_PROC_NAME
if [ "$?" != "0" ]; then
    echo "package $APP_PROC_NAME not installed! skip monitoring!"
    sleep 60
    return
fi

mainpid="`pidof $APP_PROC_NAME`"
filelocked="0"
if [ "$mainpid" != "" ]; then
    ls -al /proc/$mainpid/fd | grep "lockfile.txt"
    filelocked=$?
fi

if [ "$mainpid" == "" -o "$filelocked" != "0" ]; then
    echo "found $APP_PROC_NAME die, launch directly"
    am start -n $APP_ACTIVITY

    if [ "$?" != 0 ]; then
        echo "Fail to launch $APP_ACTIVITY, retry"
    else
        echo "Success to lauch $APP_ACTIVITY"
    fi

    #write mark the app die issue
    echo "LAUNCH APP@`get_time`" >> $APP_RELAUNCH_FILE
    sleep 60

else
    echo "Found $APP_ACTIVITY alive"
    if [ -f ${APP_RELAUNCH_FILE} ]; then
        echo "clean $APP_RELAUNCH_FILE"
        rm $APP_RELAUNCH_FILE
    fi
    sleep 60
fi

APP_DIE_LIMIT=60

if [ -f $APP_RELAUNCH_FILE ]; then
    die_cnt="`cat $APP_RELAUNCH_FILE|wc -l`"
    echo "Found app continuous die $die_cnt times"
    if [ "$die_cnt" -gt "$APP_DIE_LIMIT" ]; then
        if [ -f $DISABLE_REBOOT_FILE ]; then
            echo "Found $DISABLE_REBOOT_FILE, clean $APP_RELAUNCH_FILE, skip reboot device"
            rm -rf $APP_RELAUNCH_FILE
        else
            echo "App die for continuous $die_cnt times, clean $APP_RELAUNCH_FILE and reboot device!"
            rm -rf $APP_RELAUNCH_FILE
            sync
            reboot
        fi
    fi
fi

echo "=====>$0 end @ `get_time`"
