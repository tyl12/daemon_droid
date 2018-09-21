//
// Created by david on 7/3/18.
//

#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <wait.h>
#include <string.h>
#include "utils.h"
#include "error.h"

#define AUTOSSH_BIN_PATH "/system/bin/autossh"
#define      LOGCAT_PATH "/sdcard/iceLocker/Data/log/need_upload_logs/%s"
#define       LOCAL_PORT "22"
#define         MAX_LINE 64

static char    SERVER_IP[64] = { '\0' };
static char  SERVER_PORT[64] = { '\0' };
static char  SERVER_USER[64] = { '\0' };
static char       R_PORT[64] = { '\0' };
static char RSSH_MONITOR[64] = { '\0' };
static char    DEVICE_ID[64] = { '\0' };
static char        DEBUG[64] = { '\0' };

extern const char* const UPGRADE_PATH;
extern const char* const UPGRADE_FILE;

const char* const     ENV_PATH = "/sdcard/.environment";
const char* const     SSH_PATH = "/system/bin/ssh";
const char* const  SSH_LOGFILE = "/data/local/tmp/logfile_ssh";
const char* const  SSHD_CONFIG = "/system/etc/ssh/sshd_config";
const char* const    SSHD_PATH = "/system/bin/sshd";
const char* const UPGRADE_PATH = "/sdcard/iceLocker/upgrade/%s";
const char* const UPGRADE_FILE = "/sdcard/iceLocker/upgradeVersion";
const char* const    DATA_PATH = "/sdcard/iceLocker/IceLocker/";
const char* const   LAUNCH_APK = "am start -n com.xiaomeng.icelocker/com.xiaomeng.iceLocker.ui.activity.MainActivity";
const char* const    LOCK_FILE = "/data/data/com.xiaomeng.icelocker/files/lockfile.txt";
const char* const  STOP_DAEMON = "/sdcard/iceLocker/debug/disable_daemon";
const char* const  STOP_REBOOT = "/sdcard/iceLocker/debug/disable_reboot";

const char* const  UNINSTALL_APK = "pm uninstall -k com.xiaomeng.icelocker";
const char* const    INSTALL_APK = "pm install -r -t --user 0 %s";      /* apk name with absolute path */
const char* const LAUNCH_AUTOSSH = "%s -f "                             /* autossh cmd path */
                                   "-M %s "                             /* monitor port */
                                   "-NR %s"                             /* forward port */
                                   ":localhost:%s "                     /* default ssh port of local is 22 */
                                   "-i '/data/ssh/ssh_host_rsa_key' "   /* private key */
                                   "-o 'StrictHostKeyChecking=no' "     /* without host authentication */
                                   "-o 'ServerAliveInterval=60' "       /* send heart beat every 60s */
                                   "-o 'ServerAliveCountMax=3' "        /* check alive max count */
                                   "%s"                                 /* user name of forward server */
                                   "@%s "                               /* ip of forward server */
                                   "-p %s";                             /* ssh port of forward server */


static bool isstop()
{
    bool isexist = access(STOP_DAEMON, F_OK) != -1;
    LOG_I("[THREAD-ID:%zu]is \"%s\" available? %s", pthread_self(), STOP_DAEMON, isexist ? "true" : "false");
    return isexist;
}

void* monitor_logcat(void *arg)
{
    int status, pid;
    struct timespec ts;
    char cur_time[64], shell_cmd[1024], filename[64], logpath[256];


    char shell_cmd[] = "sh /sdcard/iceLocker/scripts/save_logcat.sh"
    for (;;)
    {
        status = exec_cmd(shell_cmd);
        if (status != 0){
            LOG_I("fail to execute cmd: %s", shell_cmd);
            sleep(30);
            continue;
        }

        /*
        clock_gettime(CLOCK_REALTIME, &ts);
        struct tm tm1 = *localtime(&ts.tv_sec);
        sprintf(cur_time, "%4d-%02d-%02d_%02dh", tm1.tm_year + 1900, tm1.tm_mon + 1, tm1.tm_mday, tm1.tm_hour & 0xFE);

        sprintf(filename, ".XM_%s.log", cur_time);
        sprintf(logpath, LOGCAT_PATH, filename);

        sprintf(shell_cmd, "timeout 2h logcat -v threadtime >> %s; logcat -c", logpath);
        exec_cmd(shell_cmd);

        FILE *fp = fopen(logpath, "a+");
        fprintf(fp, "\n########################### dmesg ###########################\n\n");
        fclose(fp);
        sprintf(shell_cmd, "dmesg -c >> %s", logpath);
        exec_cmd(shell_cmd);

        sprintf(shell_cmd, "mv %s " LOGCAT_PATH, logpath, filename + 1);
        exec_cmd(shell_cmd);
        */
    }
    return NULL;
}

void* monitor_ssh(void *arg)
{
    proc_info pinfo;
    char run_sshd[1024], run_autossh[1024];
    bool isdebug = strlen(DEBUG) == 4;

    sprintf(run_sshd, "%s -f %s", SSHD_PATH, SSHD_CONFIG);
    sprintf(run_autossh, LAUNCH_AUTOSSH, AUTOSSH_BIN_PATH, RSSH_MONITOR, R_PORT, LOCAL_PORT, SERVER_USER, SERVER_IP, SERVER_PORT);

    for (;;)
    {
        pinfo = check_proc("sshd");
        if (!pinfo.is_alive) exec_cmd(run_sshd);
        else                 LOG_I("[%s]\"%-20s\" is running with pid %d\n", __FUNCTION__, pinfo.name, pinfo.pid);

        pinfo = check_proc("autossh");
        if (!pinfo.is_alive) exec_cmd(run_autossh);
        else                 LOG_I("[%s]\"%-20s\" is running with pid %d\n", __FUNCTION__, pinfo.name, pinfo.pid);

        if (isdebug) sleep(10);
        else         sleep(60);
    }
}

int main__()
{
    pid_t pid;
    int status, try_cnt, restart_cnt;
    status = try_cnt = 0;
    restart_cnt = -1;
    proc_info pinfo;
    char shell_cmd[1024], time_s[64], apk[BUF_SIZE], pathname[BUF_SIZE];
    bool has_fd, isretry, isupd, isreboot, isdebug;
    has_fd = isretry = isupd = isreboot = false;
    FILE *fp;

    sleep(60);

    setenv("AUTOSSH_PATH",       SSH_PATH,    1);
    setenv("AUTOSSH_DEBUG",      "1",         1);
    setenv("AUTOSSH_LOGLEVEL",   "7",         1);
    setenv("AUTOSSH_LOGFILE",    SSH_LOGFILE, 1);
    setenv("AUTOSSH_POLL",       "600",       1);
    setenv("AUTOSSH_FIRST_POLL", "300",       1);

    /* get { key, value } from /sdcard/.environment */
    fp = fopen(ENV_PATH, "r");
    if (!fp)
        err_sys("cannot open .environment");

    char line[MAX_LINE];
    char *equal;
    while (fgets(line, MAX_LINE, fp))
    {
        if ((equal = strchr(line, '=')) == NULL)
            continue;

        *equal = '\0';
        if (   *SERVER_IP == '\0' && strstr(line, "RSSH_SVRIP"))   { trim(    SERVER_IP, equal + 1); continue; }
        if ( *SERVER_PORT == '\0' && strstr(line, "RSSH_SVRPORT")) { trim(  SERVER_PORT, equal + 1); continue; }
        if ( *SERVER_USER == '\0' && strstr(line, "RSSH_USER"))    { trim(  SERVER_USER, equal + 1); continue; }
        if (      *R_PORT == '\0' && strstr(line, "RSSH_PORT"))    { trim(       R_PORT, equal + 1); continue; }
        if (*RSSH_MONITOR == '\0' && strstr(line, "RSSH_MONITOR")) { trim( RSSH_MONITOR, equal + 1); continue; }
        if (   *DEVICE_ID == '\0' && strstr(line, "DEVICEID"))     { trim(    DEVICE_ID, equal + 1); continue; }
        if (       *DEBUG == '\0' && strstr(line, "DEBUG"))        { trim(        DEBUG, equal + 1); continue; }
    }

    LOG_I(">>>>>>>>>>>>>>>> environment variables >>>>>>>>>>>>>>>>>>>>>\n");
    LOG_I("%-26s = %s\n", "ENV_XIAOMENG_DEVICEID",     DEVICE_ID);
    LOG_I("%-26s = %s\n", "ENV_XIAOMENG_DEBUG",        DEBUG);
    LOG_I("%-26s = %s\n", "ENV_XIAOMENG_RSSH_MONITOR", RSSH_MONITOR);
    LOG_I("%-26s = %s\n", "ENV_XIAOMENG_RSSH_PORT",    R_PORT);
    LOG_I("%-26s = %s\n", "ENV_XIAOMENG_RSSH_USER",    SERVER_USER);
    LOG_I("%-26s = %s\n", "ENV_XIAOMENG_RSSH_SVRIP",   SERVER_IP);
    LOG_I("%-26s = %s\n", "ENV_XIAOMENG_RSSH_SVRPORT", SERVER_PORT);
    LOG_I("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n\n");

    fclose(fp);
    isdebug = strlen(DEBUG) == 4;

    pthread_t thrd_ssh, thrd_logcat;
    status += (pthread_create(&thrd_ssh, NULL, monitor_ssh, NULL) != 0);
    status += (pthread_create(&thrd_logcat, NULL, monitor_logcat, NULL) != 0);
    if (status != 0)
    {
        perror("cannot create threads");
        exit(1);
    }

    LOG_I(">>>>>>>>>>>>>>>>>>>>>>>> once jobs >>>>>>>>>>>>>>>>>>>>>>>>>\n");
    LOG_I("###\n");
    exec_cmd("sh /data/bin/once.sh");
    LOG_I("###\n");
    LOG_I("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n\n");

    for (;;)
    {
        if (isstop()) break;
        LOG_I(">>>>>>>>>>>>>>>>>>>>>>>> loop jobs >>>>>>>>>>>>>>>>>>>>>>>>>\n");
        LOG_I("###\n");
        exec_cmd("sh /data/bin/inter.sh");
        LOG_I("###\n");
        LOG_I("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n\n");

        /* is FactoryTest alive? */
        pinfo = check_proc("com.xiaomeng.androidselftest");
        if (pinfo.is_alive)
        {
            kill_proc("com.xiaomeng.icelocker");
            goto next;
        }

        /* upgrade package available? */
retry:
        *pathname = 0;
        *apk = 0;
        check_ver(pathname);
        check_apk(pathname, apk);
        if (*pathname == 0 || *apk == 0)
        {
            LOG_I("there is no available update...\n");
            goto monitor_app;
        }
        status = 0;
        sprintf(shell_cmd, INSTALL_APK, apk);
        status += exec_cmd(shell_cmd);
        if (status != 0)
        {
            LOG_I("{ \"status\" : %d, \"retry_cnt\" : %d } fail to install \"%s\"", status, try_cnt, apk);
            if (++try_cnt > 10)
            {
                get_time(time_s);
                fp = fopen("/sdcard/reboot_log", "a+");
                if (fp)
                {
                    fprintf(fp, "%s upgrade failed, reboot system\n", time_s);
                    fclose(fp);
                }

                /* test */
                // chdir(pathname);
                // system("cp IceLocker_xiaomeng_v2.0_2.apk.bak IceLocker_xiaomeng_v2.0_2.apk");
                // system("mv test.apk test.apk.bak");
                // sprintf(shell_cmd, "touch %s", STOP_DAEMON);
                // exec_cmd(shell_cmd);
                system("reboot");
            }
            sleep(5);
            goto retry;
        }

        sprintf(shell_cmd, "cp -r %s/* %s", pathname, DATA_PATH);
        exec_cmd(shell_cmd);
        cls_file(UPGRADE_FILE);
        isupd = true;

monitor_app:
        /* is IceLocker alive? */
        pinfo = check_proc("com.xiaomeng.icelocker");
        has_fd = check_fd(LOCK_FILE);
        if (!pinfo.is_alive || !has_fd)
        {
            fp = fopen(STOP_REBOOT, "r");
            isreboot = fp ? false : true;
            isreboot || fclose(fp) || (restart_cnt = -1);

            if (isupd || !isreboot)
                goto launch_app;

            if (++restart_cnt >= 10)
            {
                char cur_time[64];
                get_time(cur_time);
                fp = fopen("/sdcard/reboot_log", "a+");
                if (fp)
                {
                    fprintf(fp, "%s IceLocker has crashed at least 10 times, reboot system\n", cur_time);
                    fclose(fp);
                }
                system("reboot");
            }
            LOG_I("[%s]restart_cnt = %d\n", __FUNCTION__, restart_cnt);

launch_app:
            exec_cmd(LAUNCH_APK);
            isupd = false;
        }
        else
        {
            LOG_I("[%s]\"%s\" is running with pid %d\n", __FUNCTION__, pinfo.name, pinfo.pid);
        }
next:
        if (isdebug) sleep(10);
        else         sleep(60);
    }

    pthread_join(thrd_ssh, NULL);
    pthread_join(thrd_logcat, NULL);

    return 0;
}
