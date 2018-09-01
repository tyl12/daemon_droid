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

static int  restart_cnt = 0; /* autossh should be launched after network */

extern const char* const UPGRADE_PATH;
extern const char* const UPGRADE_FILE;
extern const char* const LAUNCH_AUTOSSH;

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

const char* const  UNINSTALL_APK = "pm uninstall -k com.xiaomeng.icelocker";
const char* const  INSTALL_APK   = "pm install -r -t --user 0 %s";      /* apk name with absolute path */
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

void* monitor_logcat(void *arg)
{
    int status, pid;
    struct timespec ts;
    char cur_time[64];
    char shell_cmd[1024];
    char filename[64];
    char logpath[256];
    for (;;)
    {
        clock_gettime(CLOCK_REALTIME, &ts);
        struct tm tm1 = *localtime(&ts.tv_sec);
        sprintf(cur_time, "%4d-%02d-%02d_%02dh", tm1.tm_year + 1900, tm1.tm_mon + 1, tm1.tm_mday, tm1.tm_hour);

        sprintf(filename, ".XM_%s.log", cur_time);
        sprintf(logpath, LOGCAT_PATH, filename);

        sprintf(shell_cmd, "logcat -d -t 200 -f %s", logpath); /* backup */
        exec_cmd(shell_cmd);
        strcpy(shell_cmd, "logcat -c"); /* clear */
        exec_cmd(shell_cmd);

        sprintf(shell_cmd, "timeout 2h logcat -s >> %s", logpath);
        LOG_I("[%s]exec: \"%s\"", __FUNCTION__, shell_cmd);
        char **cmd_vec = calloc(1, BUF_SIZE);
        list2vec(shell_cmd, cmd_vec);

        if ((pid = fork()) < 0)
        {
            err_sys("fork error");
        }
        else if (pid == 0) /* child process */
        {
            execvp("/system/bin/timeout", cmd_vec);
            _Exit(127);
        }
        else
        {
            waitpid(pid, &status, 0);
            pr_exit(status); /* exit with 142 */

            FILE *fp = fopen(logpath, "a+");
            fprintf(fp, "\n########################### dmesg ###########################\n\n");
            fclose(fp);
            sprintf(shell_cmd, "dmesg -c >> %s", logpath);
            exec_cmd(shell_cmd);

            sprintf(shell_cmd, "mv %s " LOGCAT_PATH, logpath, filename + 1);
            exec_cmd(shell_cmd);
        }

        free(cmd_vec);
    }
}

void* monitor_app(void *arg)
{
    pid_t pid;
    int status, restart_cnt;
    proc_info info;
    proc_info selftest_info;
    char shell_cmd[1024];
    restart_cnt = -1;
    for (;;)
    {
        selftest_info = check_proc("com.xiaomeng.androidselftest");
        info = check_proc("com.xiaomeng.icelocker");
        if(selftest_info.is_alive)
        {
            if(info.is_alive)
            {
                kill_proc("com.xiaomeng.icelocker");
            }
            sleep(10);
            continue;
        }

        bool has_fd = check_fd(LOCK_FILE);
        if (!info.is_alive || !has_fd)
        {
            FILE* fp = fopen("/sdcard/disable_reboot", "r");
            bool is_reboot = fp ? false : true;
            is_reboot || fclose(fp);

            if (is_reboot && ++restart_cnt >= 10)
            {
                char cur_time[64];
                get_time(cur_time);
                fp = fopen("/sdcard/reboot_log", "a+");
                fprintf(fp, "%s reboot system\n", cur_time);
                fclose(fp);
                system("reboot");
            }
            printf("[%s]restart_cnt = %d\n", __FUNCTION__, restart_cnt);

            char **cmd_vec = calloc(1, BUF_SIZE);
            strcpy(shell_cmd, LAUNCH_APK);
            LOG_I("### exec: \"%s\"\n", shell_cmd);
            list2vec(shell_cmd, cmd_vec);

            if ((pid = fork()) < 0)
            {
                err_sys("fork error");
            }
            else if (pid == 0) /* child process */
            {
                execvp("/system/bin/am", cmd_vec);
                _Exit(127);
            }
            else
            {
                waitpid(pid, &status, 0);
                pr_exit(status);
            }

            free(cmd_vec);
        }
        else
        {
            LOG_I("[%s]\"%s\" is running with pid %d\n", __FUNCTION__, info.name, info.pid);
        }

        if (strlen(DEBUG) == 4)
            sleep(10);
        else
            sleep(60);
    }
}

void* monitor_ssh(void *arg)
{
    int timer, status, restart_cnt = 0;
    proc_info info;
    char shell_cmd[1024];
    bool isdebug = strlen(DEBUG) == 4 ? true : false;

    for (;;)
    {
        info = check_proc("sshd");
        if (!info.is_alive)
        {
            sprintf(shell_cmd, "%s -f %s", SSHD_PATH, SSHD_CONFIG);
            LOG_I("### exec: \"%s\"\n", shell_cmd);
            status = system(shell_cmd);

            if (status == -1)  { LOG_I("fork fails or waitpid returns an error other than EINTR: %s", strerror(errno)); }
            if (status == 127) { LOG_I("exec fails"); }
        }
        else
        {
            LOG_I("[%s]\"%s\" is running with pid %d\n", __FUNCTION__, info.name, info.pid);
        }

        info = check_proc("autossh");
        if (!info.is_alive)
        {
            sprintf(shell_cmd, LAUNCH_AUTOSSH, AUTOSSH_BIN_PATH, RSSH_MONITOR, R_PORT, LOCAL_PORT, SERVER_USER, SERVER_IP, SERVER_PORT);
            LOG_I("### exec: \"%s\"\n", shell_cmd);
            status = system(shell_cmd);

            if (status == -1)  { LOG_I("fork fails or waitpid returns an error other than EINTR: %s", strerror(errno)); }
            if (status == 127) { LOG_I("exec fails"); }
        }
        else
        {
            LOG_I("[%s]\"%s\" is running with pid %d\n", __FUNCTION__, info.name, info.pid);

            if (!isdebug && timer++ > 60)
            {
                timer = 0;
                goto restart;
            }

            if (restart_cnt == 0) /* exec regularly */
            {
                static int countdown = 0;
                if (countdown++ < 6)
                {
                    LOG_I("### [%02d] keep ssh family alive", countdown);
                    goto next;
                }

restart:
                LOG_I("### [%d] restart ssh family to make sure proxy is available (critical)\n", ++restart_cnt);
                kill_proc("autossh");
                kill_proc("ssh");
                kill_proc("sshd");

                if (restart_cnt > 0x0fffffff)
                    restart_cnt = 0;
            }
        }
next:
        if (strlen(DEBUG) == 4)
            sleep(10);
        else
            sleep(60);
    }
}

int main()
{
    pid_t pid;
    int status;
    proc_info info;
    char shell_cmd[1024];
    char time_s[64];
    FILE *f;

    setenv("AUTOSSH_PATH", SSH_PATH, 1);
    setenv("AUTOSSH_DEBUG", "1", 1);
    setenv("AUTOSSH_LOGLEVEL", "7", 1);
    setenv("AUTOSSH_LOGFILE", SSH_LOGFILE, 1);

    /* get { key, value } from /sdcard/.environment */
    f = fopen(ENV_PATH, "r");
    if (!f)
        err_sys("cannot open .environment");

    char line[MAX_LINE];
    char *equal;
    while (fgets(line, MAX_LINE, f))
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

    fclose(f);

    pthread_t thrd_app, thrd_ssh, thrd_logcat;
    status = (pthread_create(&thrd_app, NULL, monitor_app, NULL) != 0);
    status += (pthread_create(&thrd_ssh, NULL, monitor_ssh, NULL) != 0);
    status += (pthread_create(&thrd_logcat, NULL, monitor_logcat, NULL) != 0);
    if (status != 0)
    {
        perror("cannot create threads");
        exit(1);
    }

#if 1
    for (;;)
    {
        get_time(time_s);
        LOG_I(">>>>>>>>>>>>>>>>>>>>>>> other jobs >>>>>>>>>>>>>>>>>>>>>>>>> %s\n", time_s);
        LOG_I("###\n");
        exec_cmd("sh /data/bin/inter.sh");
        LOG_I("###\n");
        LOG_I("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n\n");

        get_time(time_s);
        LOG_I(">>>>>>>>>>>>>>>>>>>>> check update >>>>>>>>>>>>>>>>>>>>>>>>> %s\n", time_s);
        LOG_I("###\n");
        char *apk = calloc(1, BUF_SIZE);
        char pathname[256] = { '\0' };
        check_ver(pathname);
        check_apk(pathname, apk);
        if (*pathname == 0 || *apk == 0)
        {
            LOG_I("### there is no available update...\n");
            goto next;
        }

        /**
         * new apk found:
         * 1. uninstall old apk (with -k)
         * 2. install new apk
         * 3. copy /sdcard/iceLocker/upgrade/IceLocker_versionxxx/data_xm to DATA_PATH
         * 4. launch com.xiaomeng.icelocker
         */
        system(UNINSTALL_APK); /* STEP 1 */

        /* STEP 2 */
        sprintf(shell_cmd, INSTALL_APK, apk);
        system(shell_cmd);

        /* STEP 3 */
        char *cur_dir = calloc(1, BUF_SIZE);
        getcwd(cur_dir, BUF_SIZE);
        LOG_I("### current work directory: %s\n", cur_dir);
        LOG_I("### change dir to: %s\n", pathname);
        chdir(pathname);
        sprintf(shell_cmd, "cp -rf data_xm %s", DATA_PATH);
        LOG_I("### %s\n", shell_cmd);
        system(shell_cmd);
        chdir(cur_dir);
        free(cur_dir);

        system(LAUNCH_APK);    /* STEP 4 */
next:
        free(apk);
        LOG_I("###\n");
        LOG_I("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n\n");
        if (strlen(DEBUG) == 4)
            sleep(10);
        else
            sleep(60);
    }
#endif

    /* thrd_app & thrd_ssh never join */
    pthread_join(thrd_app, NULL);
    pthread_join(thrd_ssh, NULL);

    return 0;
}
