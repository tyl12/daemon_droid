. $ENV_DEEPLEARN_SCRIPT_DIR/utils_ro.sh
echo "=====>$0 begin @ `get_time`"
TAG="$0 : "

sleep 60

pinfo=`ps|grep -i client_droid`
pinfo=`echo $pinifo`
if [ "$pinfo" == "" ]; then
    echo "client_droid not launch, skip monitor this time"
    sleep 20
else
    echo "client_droid alive, check status"
    if [ -f /data/local/tmp/status ]; then
        echo "

fi

echo "=====>$0 end @ `get_time`"
