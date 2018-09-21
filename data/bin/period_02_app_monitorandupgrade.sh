. $ENV_XIAOMENG_SCRIPT_DIR/utils.sh
echo "=====>$0 begin @ `get_time`"
TAG="$0 : "


APP_PROC_NAME="com.xiaomeng.icelocker"
FACTORY_PROC_NAME="com.xiaomeng.androidselftest"

APP_ACTIVITY="com.xiaomeng.icelocker/com.xiaomeng.iceLocker.ui.activity.MainActivity"

UPGRADE_DIR="/sdcard/iceLocker/upgrade"
UPGRADE_FLAG="/sdcard/iceLocker/upgradeVersion"

WORK_PATH="/sdcard/iceLocker/IceLocker"

factorypid="`pidof $FACTORY_PROC_NAME`"
mainpid="`pidof $APP_PROC_NAME`"

#***************************************************************
#if factorytest app is alive,
#   kill main app.
#   sleep 10sec
#   return
#else
#   if upgradeVersion file is available and checksum pass
#       kill main app and do upgrade
#       sleep 10s
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

if [ "$factorypid" != "" ]; then
    echo "${TAG}Found $FACTORY_PROC_NAME alive, kill $APP_PROC_NAME for factory test"
    if [ "$mainpid" != "" ]; then
        kill -9 $mainpid
    else
        echo "${TAG}$APP_PROC_NAME is alread die, continue"
    fi
    sleep 10
    return
else
    if [ -f $UPGRADE_FLAG ]; then
        upgrade_cont="`head -n 1 $UPGRADE_FLAG`"
        #remove tailing whitespace if any
        upgrade_cont=`echo $upgrade_cont`
        if [ "$upgrade_cont" != "" ]; then
            update_target="$UPGRADE_DIR/$upgrade_cont"
            echo "${TAG}Found contents: $upgrade_cont in $UPGRADE_FLAG. search for $update_target"
            if [ -d ${update_target} ]; then
                echo "${TAG}Found matching $update_target. do update..."
                #kill main app first
                if [ "$mainpid" != "" ]; then
                    echo "${TAG}Found $APP_PROC_NAME alive, kill first for update"
                    kill -9 $mainpid
                    sleep 10
                fi

                echo "${TAG}copy $update_target -> $WORK_PATH"
                mkdir -p $WORK_PATH
                rm -rf $WORK_PATH
                cp -r $update_target  $WORK_PATH
                cp $UPGRADE_FLAG  $WORK_PATH/
                chmod 777 $WORK_PATH -R
                sync

                apklist="`ls $WORK_PATH/*.apk`"
                for apk in "$apklist" ; do
                    echo "${TAG}install $apk"
                    pm install -r -t --user 0 $WORK_PATH/${apk}.apk
                    if [ "$?" != "0" ]; then
                        echo "${TAG}fail to install $apk, exit for next retry"
                        return
                    else
                        echo "${TAG}success to install $apk"
                        echo "" > $UPGRADE_FLAG
                        sleep 5
                    fi
                done
            else
                echo "${TAG}Invalid $update_target. something wrong..."
                return
            fi
        fi
    fi

    if [ "$mainpid" == "" ]; then
        echo "${TAG}found $APP_PROC_NAME die, launch directly"
        am start -n $APP_ACTIVITY
        if [ "$?" != 0 ]; then
            echo "${TAG}Fail to launch $APP_ACTIVITY, retry"
            sleep 10
        else
            echo "${TAG}Success to lauch $APP_ACTIVITY"
            sleep 60
        fi
    else
        echo "${TAG}Found $APP_ACTIVITY alive"
        sleep 60
    fi

fi

echo "=====>$0 end @ `get_time`"
