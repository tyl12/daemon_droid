. $ENV_DEEPLEARN_SCRIPT_DIR/utils_ro.sh
echo "=====>$0 begin @ `get_time`"

TMP_PATH="/data/local/tmp"

chmod a+x $TMP_PATH/*.sh

chmod 777 $TMP_PATH/*.jpg
chmod 777 $TMP_PATH/*.jpeg
chmod 777 $TMP_PATH/out* -R


sleep 120

echo "=====>$0 end @ `get_time`"
