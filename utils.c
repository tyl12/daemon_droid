//
// Created by david on 7/3/18.
//

#include "utils.h"
#include "error.h"
#include <stdio.h>
#include <sys/wait.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>

extern const char* const UPGRADE_PATH;
extern const char* const DATA_LINK;


void pr_exit(int status)
{
    if (WIFEXITED(status))
    {
        printf("normal termination, exit status = %d\n", WEXITSTATUS(status));
    }
    else if (WIFSIGNALED(status))
    {
        printf("abnormal termination, signal number = %d%s\n", WTERMSIG(status),
#ifdef WCOREDUMP
               WCOREDUMP(status) ? " (core file generated)" : "");
#else
               "");
#endif
    }
    else if (WIFSTOPPED(status))
    {
        printf("child stopped, signal number = %d\n", WSTOPSIG(status));
    }
}

static void mkdirp(const char *filepath)
{
    char *shell_cmd = calloc(1, BUF_SIZE);
    sprintf(shell_cmd, "mkdir -p %s", filepath);
    system(shell_cmd);
    free(shell_cmd);
}

void check_apk(char *apk)
{
    struct dirent *file;
    DIR *dir;

    // mkdirp(UPGRADE_PATH); /* make sure upgrade path is existed */ (SIGSEGV may occur)
    dir = opendir(UPGRADE_PATH);
    if (!dir) /* UPGRADE_PATH do not exist */
        return;

    char *name = (char*)malloc(256);
    memset(name, 0, 256);
    while ((file = readdir(dir)))
    {
        char *fname = file->d_name;
        if (strstr(fname, "apk"))
        {
            char *tmp1, *tmp2;
            tmp1 = tmp2 = fname;
            /* to distinguish multiple "apk" contained in file name */
            while ((tmp1 = strstr(tmp1 + 1, "apk")))
            {
                tmp2 = tmp1;
            }

            if (tmp2[3] != '\0')
            {
                continue;
            }

            sprintf(name, "%s/%s", UPGRADE_PATH, fname);

            struct stat st;
            lstat(name, &st);

            if ((st.st_mode & S_IFMT) == S_IFREG) /* make sure *.apk is a regular file */
            {
                strcpy(apk, name);
                break;
            }
        }
    }

    free(name);
    closedir(dir);
}

int link_data()
{
    int state;
    struct dirent *file;
    DIR *dir;
    dir = opendir(UPGRADE_PATH);

    char name[256] = { '\0' };
    char *tname = malloc(256);
    while ((file = readdir(dir)))
    {
        char *fname = file->d_name;
        if ((fname = strstr(fname, "data_xm")))
        {
            if (fname != file->d_name)
            {
                continue;
            }

            sprintf(tname, "%s/%s", UPGRADE_PATH, fname);

            struct stat st;
            lstat(tname, &st);
            if ((st.st_mode & S_IFMT) != S_IFDIR)
            {
                continue;
            }

            memcpy(name, tname, strlen(tname) + 1);
            break;
        }
    }

    if (*name == '\0')
    {
        return -2; /* not found */
    }


    /* remove old link data */
    sprintf(tname, "rm -rf %s", DATA_LINK);
    system(tname);
    free(tname);

    /* create symbolic file */
    state = symlink(name, DATA_LINK);

    return state;
}

void list2vec(char *list, char **vec)
{
    if (list == NULL)
    {
        return;
    }

    char *token;
    int idx = 0;
    const static char* delim = "\t ";

    token = strtok(list, delim);
    while (token != NULL)
    {
        vec[idx++] = token;
        token = strtok(NULL, delim);
    }

    vec[idx] = (char *) 0; /* arguments vector should be end with NULL */
}


proc_info check_proc(const char *proc_name)
{
    static const char *PROC_DIR = "/proc";
    size_t task_size = 1024;
    proc_info info = { "", false, -1 };
    char *cmdline_f = calloc(1, task_size);
    char *task_name = calloc(1, task_size);

    DIR *dir = opendir(PROC_DIR);

    if (dir != NULL)
    {
        struct dirent *file = { 0 }; /* same effect as memset(..) */
        while ((file = readdir(dir)) != 0)
        {
            int pid;
            int res = sscanf(file->d_name, "%d", &pid);

            if (res == 1) /* a valid pid */
            {
                sprintf(cmdline_f, "%s/%d/cmdline", PROC_DIR, pid);
                FILE *fp = fopen(cmdline_f, "r");
                if (!fp)
                {
                    printf("### fopen(\"%s\") failed\n", cmdline_f);
                    continue;
                }

                if (getline(&task_name, &task_size, fp)) /* realloc may occur */
                {
                    /* hit the target */
                    if (strstr(task_name, proc_name) != NULL)
                    {
                        if (strstr(task_name, "service")) /* com.xiaomeng.icelocker.service is ambiguous, keep searching */
                            continue;

                        snprintf(info.name, sizeof(info.name), "%s", task_name);
                        info.name[sizeof(info.name) - 1] = '\0'; /* make sure info.name is a null-terminated string */
                        info.is_alive = true;
                        info.pid = pid;

                        fclose(fp);
                        goto quit;
                    }
                }
                fclose(fp);
            }
        }
    }
quit:
    closedir(dir);
    free(cmdline_f);
    free(task_name);
    return info;
}

void get_pid(pid_t *pid, const char *proc_name)
{
    char *shell_cmd = calloc(1, BUF_SIZE);
    char *content = calloc(1, BUF_SIZE);
    sprintf(shell_cmd, "pidof %s", proc_name);

    FILE *fp = popen(shell_cmd, "r");
    if (fp == NULL)
    {
        err_sys("popen(..) failed");
    }

    fgets(content, BUF_SIZE, fp);
    if (content[0] != '\0')
    {
        sscanf(content, "%d", pid);
    }


    free(shell_cmd);
    free(content);
}

void get_time(char *time_s)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    struct tm tm1 = *localtime(&ts.tv_sec);
    int64_t nanosec = ts.tv_nsec / 1000000 % 1000;

    sprintf(time_s, "[%02d/%02d-%02d:%02d:%02d.%03ld]", tm1.tm_mon + 1, tm1.tm_mday, tm1.tm_hour, tm1.tm_min, tm1.tm_sec, nanosec);
}

/**
 * remove the whitespace characters from the head and tail
 * @param out: null-terminated string to be outputted (user-defined)
 * @param src: null-terminated string to be trimmed
 * @return: size of trimmed string
 */
size_t trim(char *out, const char *src)
{
    if (!src || *src == '\0')             /* NULL pointer or empty string */
        return 0;

    while (isspace((unsigned char)*src))
        ++src;

    if (*src == '\0')                     /* src is filled with whitespace? */
        return 0;

    strcpy(out, src);
    strcat(out, "\n");                    /* make sure out is end with '\n' (one of whitespace) */

    char *end = out;
    while (!isspace((unsigned char)*end))
        ++end;

    *end = '\0';

    return end - out;
}

void kill_proc(const char *name)
{
    static char shell_cmd[256];
    pid_t pid  = -1;
    get_pid(&pid, name);
    if (pid != -1)
    {
        sprintf(shell_cmd, "kill -9 %d", pid);
        LOG_I("### KILL \"%-7s\" by exec: \"%s\"", name, shell_cmd);
        system(shell_cmd);
    }
}

void lowerstr(char *str)
{
    for (char *p = str; *p; ++p)
        *p = ((*p >= 'A') && (*p <= 'Z')) ? (*p | 0x60) : *p;
}

void getwchan(char *wchan, pid_t pid) /* waitting channel */
{
    static const int len = 32;
    static const char *pat = "/proc/%d/wchan";
    char *filepath = (char*)malloc(len);
    char *str = (char*)malloc(len);
    memset(str, 0, len*sizeof(char));
    sprintf(filepath, pat, pid);
    FILE *fp = fopen(filepath, "r");
    if (!fp)
    {
        LOG_I("### cannot open \"%s\": %s", filepath, strerror(errno));
        return;
    }

    char *ret = fgets(str, len, fp);
    if (!ret)
    {
        LOG_I("### read \"%s\" failed: %s", filepath, strerror(errno));
        return;
    }

    LOG_I("### [%d][wchan] - %s\n", pid, str);
    if (wchan)
        strcpy(wchan, str);

    free(filepath);
    free(str);
    fclose(fp);
}
