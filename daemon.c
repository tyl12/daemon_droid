//
// Created by david on 7/3/18.
//

#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <wait.h>
#include <string.h>
#include <errno.h>
#include "utils.h"
#include "error.h"

#define AUTOSSH_BIN_PATH "/system/bin/autossh"
#define   LOCAL_PORT "22"
#define     MAX_LINE 64

static char    SERVER_IP[64] = { '\0' };
static char  SERVER_PORT[64] = { '\0' };
static char  SERVER_USER[64] = { '\0' };
static char       R_PORT[64] = { '\0' };
static char RSSH_MONITOR[64] = { '\0' };
static char    DEVICE_ID[64] = { '\0' };
static char        DEBUG[64] = { '\0' };

static int  restart_cnt = 0; /* autossh should be launched after network */

extern const char* const UPGRADE_PATH;
extern const char* const DATA_LINK;
extern const char* const LAUNCH_AUTOSSH;

/* sdcard: DOS filesystem?! */
const char* const     ENV_PATH = "/sdcard/.environment";
const char* const     SSH_PATH = "/system/bin/ssh";
const char* const  SSH_LOGFILE = "/data/local/tmp/logfile_ssh";
const char* const  SSHD_CONFIG = "/system/etc/ssh/sshd_config";
const char* const    SSHD_PATH = "/system/bin/sshd";
const char* const UPGRADE_PATH = "/data/data/com.xiaomeng.icelocker/files/xm/upgrade/IceLocker"; /* an absolute path */
const char* const  BACKUP_PATH = "/data/data/com.xiaomeng.icelocker/files/xm/upgrade/upgraded";
const char* const    DATA_LINK = "/data/data/com.xiaomeng.icelocker/files/data_xm";
const char* const   LAUNCH_APK = "am start -n com.xiaomeng.icelocker/com.xiaomeng.iceLocker.ui.activity.MainActivity";
const char* const  INSTALL_APK = "pm install -r -t --user 0 "
                                 "%s";                                                           /* apk name with absolute path */

const char* const  UNINSTALL_APK = "pm uninstall -k com.xiaomeng.icelocker";

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


int main()
{
    pid_t pid;
    int status;
    int timer;
    proc_info info;
    char shell_cmd[256];
    char time_s[64];
    FILE *f;
    bool isdebug;

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
    printf("+++ [%s] you have reached line %d\n", __FUNCTION__, __LINE__);
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
        // {
        //     char *val = malloc(8 * sizeof(char));
        //     size_t size;
        //     size = trim(val, equal + 1);
        //     DEBUG = (size == 4) ? 1 : 0; /* 5 - size */
        //     free(val);
        // }
    }

    get_time(time_s);
    LOG_I(">>>>>>>>>>>>>>>> environment variables >>>>>>>>>>>>>>>>>>>>> %s\n", time_s);
    LOG_I("%-26s = %s\n", "ENV_XIAOMENG_DEVICEID",     DEVICE_ID);
    LOG_I("%-26s = %s\n", "ENV_XIAOMENG_DEBUG",        DEBUG);
    LOG_I("%-26s = %s\n", "ENV_XIAOMENG_RSSH_MONITOR", RSSH_MONITOR);
    LOG_I("%-26s = %s\n", "ENV_XIAOMENG_RSSH_PORT",    R_PORT);
    LOG_I("%-26s = %s\n", "ENV_XIAOMENG_RSSH_USER",    SERVER_USER);
    LOG_I("%-26s = %s\n", "ENV_XIAOMENG_RSSH_SVRIP",   SERVER_IP);
    LOG_I("%-26s = %s\n", "ENV_XIAOMENG_RSSH_SVRPORT", SERVER_PORT);
    LOG_I("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n\n");

    fclose(f);
    isdebug = strlen(DEBUG) == 4 ? true : false;
#if 1
    for (;;)
    {
        get_time(time_s);
        LOG_I(">>>>>>>>>>>>>>>>>>>>>>> other jobs >>>>>>>>>>>>>>>>>>>>>>>>> %s\n", time_s);
        LOG_I("###\n");
        LOG_I("### exec: \"sh /system/etc/inter.sh\"");
        system("sh /system/etc/inter.sh");
        LOG_I("###\n");
        LOG_I("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n\n");

        get_time(time_s);
        LOG_I(">>>>>>>>>>>>>>>>>>>>> check sshd >>>>>>>>>>>>>>>>>>>>>>>>>>> %s\n", time_s);
        LOG_I("###\n");
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
            LOG_I("### \"%s\" is running with pid %d\n", info.name, info.pid);
        }
        LOG_I("###\n");
        LOG_I("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n\n");

        get_time(time_s);
        LOG_I(">>>>>>>>>>>>>>>>>>>>> check autossh >>>>>>>>>>>>>>>>>>>>>>>> %s\n", time_s);
        LOG_I("###\n");
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
            LOG_I("### \"%s\" is running with pid %d\n", info.name, info.pid);

            /* plan B */
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
                    goto alive;
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
alive:
        LOG_I("###\n");
        LOG_I("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n\n");

        get_time(time_s);
        LOG_I(">>>>>>>>>>>>>>>>>>>>> check IceLocker >>>>>>>>>>>>>>>>>>>>>> %s\n", time_s);
        LOG_I("###\n");
        info = check_proc("com.xiaomeng.icelocker");
        if (!info.is_alive)
        {
            char **cmd_vec = calloc(1, BUF_SIZE);

            sprintf(shell_cmd, LAUNCH_APK);
            list2vec(shell_cmd, cmd_vec);

            LOG_I("### exec: \"%s\"\n", shell_cmd);
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
            LOG_I("### \"%s\" is running with pid %d\n", info.name, info.pid);
#if 0
            char *wchan = calloc(32, sizeof(char));
            getwchan(wchan, info.pid);
            if (*wchan)
            {
                lowerstr(wchan);
                if (strstr(wchan, "futex") == wchan)
                {
                    LOG_I("### restart com.xiaomeng.icelocker");
                    kill(info.pid, SIGKILL);
                }
            }
            free(wchan);
#endif
        }
        LOG_I("###\n");
        LOG_I("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n\n");

        get_time(time_s);
        LOG_I(">>>>>>>>>>>>>>>>>>>>> check update >>>>>>>>>>>>>>>>>>>>>>>>> %s\n", time_s);
        LOG_I("###\n");
        char *apk = calloc(1, BUF_SIZE);
        check_apk(apk);
        if (apk[0] == '\0')
        {
            LOG_I("### there is no available update...\n");
            goto next;
        }

        /**
         * new apk found:
         * 1. uninstall old apk (with -k)
         * 2. install new apk
         * 3. creak symbolic for data_xm
         * 4. move 'old' apk to /data/data/com.xiaoemng.icelocker/files/upgraded
         * 5. launch com.xiaomeng.icelocker
         */
        system(UNINSTALL_APK); /* STEP 1 */

        /* STEP 2 */
        sprintf(shell_cmd, INSTALL_APK, apk);
        system(shell_cmd);

        status = link_data();           /* STEP 3 */
        switch(status)
        {
            case -1:
                LOG_I("### cannot make soft link: %s", strerror(errno));
                break;
            case -2:
                LOG_I("### %s/data_xm do not exist", UPGRADE_PATH);
                break;
            default:
                break;
        }

        /* STEP 4 */
        char *cur_dir = calloc(1, BUF_SIZE);
        getcwd(cur_dir, BUF_SIZE);
        LOG_I("### current work directory: %s\n", cur_dir);
        LOG_I("### change dir to: %s\n", UPGRADE_PATH);
        chdir(UPGRADE_PATH);
        char *apk_name = strrchr(apk, '/') + 1;
        // system("mkdir -p ../upgraded"); /* make sure dir "../upgraded" is available */
        sprintf(shell_cmd, "mv %s ../upgraded/", apk_name);
        LOG_I("### %s\n", shell_cmd);
        system(shell_cmd);
        chdir(cur_dir);
        free(cur_dir);

        system(LAUNCH_APK);    /* STEP 5 */
next:
        free(apk);
        get_time(time_s);
        LOG_I("###\n");
        LOG_I("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n\n");
        if (strlen(DEBUG) == 4)
            sleep(10);
        else
            sleep(60);
    }
#endif

    return 0;
}
