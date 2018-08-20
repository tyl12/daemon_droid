#!/system/bin/sh

set -ex

if [[ $1 = "" ]]
then
    echo "usage: sh $0 path/to/xiaomengDaemon"
    exit 1
fi

OLD_DAEMON="/system/bin/xiaomengDaemon"
NEW_DAEMON=$1
name=${NEW_DAEMON:0-14}
if [[ $name != "xiaomengDaemon" ]]
then
    echo "invalid name"
    exit 1
fi

if [[ ! -x $NEW_DAEMON ]]
then
    echo "invalid file"
    exit 1
fi

mount -o rw,remount /system
sleep 1
mount | grep system
sleep 1
sync
sleep 1

rm $OLD_DAEMON
cp $NEW_DAEMON /system/bin/
NEW_DAEMON=$OLD_DAEMON
chmod 755 $NEW_DAEMON
chown root:shell $NEW_DAEMON
sleep 1

restorecon $NEW_DAEMON
sleep 1

reboot
