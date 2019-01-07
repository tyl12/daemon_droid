
#adb root
#adb disable-verity
#adb remount

adb push ../libs/arm64-v8a/client_droid /system/bin/client_droid
adb push ../libs/arm64-v8a/client_droid_internal /system/bin/client_droid_internal
#adb push ../script/ /data/local/tmp/
for i in `ls ../script`; do
    adb push ../script/$i  /data/local/tmp/
done

adb shell sync

