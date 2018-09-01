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
#include <fcntl.h>
#include <pthread.h>

extern const char* const UPGRADE_PATH;
extern const char* const UPGRADE_FILE;

void pr_exit(int status)
{
    if (WIFEXITED(status))
    {
        LOG_I("[THREAD-ID:%zu]normal termination, exit status = %d\n", pthread_self(), WEXITSTATUS(status));
    }
    else if (WIFSIGNALED(status))
    {
        LOG_I("[THREAD-ID:%zu]abnormal termination, signal number = %d%s\n", pthread_self(), WTERMSIG(status),
#ifdef WCOREDUMP
               WCOREDUMP(status) ? " (core file generated)" : "");
#else
               "");
#endif
    }
    else if (WIFSTOPPED(status))
    {
        LOG_I("[THREAD-ID:%zu]child stopped, signal number = %d\n", pthread_self(), WSTOPSIG(status));
    }
}

static void mkdirp(const char *filepath)
{
    char *shell_cmd = calloc(1, BUF_SIZE);
    sprintf(shell_cmd, "mkdir -p %s", filepath);
    system(shell_cmd);
    free(shell_cmd);
}

static bool is_dir(const char *path)
{
    struct stat statbuf;
    if (lstat(path, &statbuf) == -1)
    {
        LOG_I("[%s]lstat error: %s", __FUNCTION__, strerror(errno));
        return false;
    }

    switch (statbuf.st_mode & S_IFMT)
    {
        case S_IFBLK:  LOG_I("[%s]\"%s\" is %s", __FUNCTION__, path, "block device");           break;
        case S_IFCHR:  LOG_I("[%s]\"%s\" is %s", __FUNCTION__, path, "character device");       break;
        case S_IFDIR:  LOG_I("[%s]\"%s\" is %s", __FUNCTION__, path, "directory"); return true; break;
        case S_IFIFO:  LOG_I("[%s]\"%s\" is %s", __FUNCTION__, path, "FIFO/pipe");              break;
        case S_IFLNK:  LOG_I("[%s]\"%s\" is %s", __FUNCTION__, path, "symlink");                break;
        case S_IFREG:  LOG_I("[%s]\"%s\" is %s", __FUNCTION__, path, "regular file");           break;
        case S_IFSOCK: LOG_I("[%s]\"%s\" is %s", __FUNCTION__, path, "socket");                 break;
        default:       LOG_I("[%s]\"%s\" is %s", __FUNCTION__, path, "unknown?");               break;
    }
    return false;
}

void cls_file(const char *filepath)
{
    fclose(fopen(filepath, "w"));
}

void check_apk(const char *path, char *apk)
{
    struct dirent *file;
    DIR *dir;

    dir = opendir(path);
    if (!dir)
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
            while ((tmp1 = strstr(tmp1 + 1, "apk")))
            {
                tmp2 = tmp1;
            }

            if (tmp2[3] != '\0')
            {
                continue;
            }

            sprintf(name, "%s/%s", path, fname);

            struct stat st;
            lstat(name, &st);

            if ((st.st_mode & S_IFMT) == S_IFREG)
            {
                strcpy(apk, name);
                break;
            }
        }
    }

    free(name);
    closedir(dir);
}

void check_ver(char *pathname)
{
    int fd, nread;
    LOG_I("[%s]check \"%s\"", __FUNCTION__, UPGRADE_FILE);
    if ((fd = open(UPGRADE_FILE, O_RDONLY)) == -1)
    {
        LOG_I("[%s]open upgrade file failed: %s", __FUNCTION__, strerror(errno));
        return;
    }

    char buf[256] = { '\0' };
    nread = read(fd, buf, 256);
    trim(buf, buf);
    LOG_I("[%s]upgrade contents: \"%s\"", __FUNCTION__, buf);
    if (*buf == '\0')
        goto end;

    const char *slash = strrchr(buf, '/');

    sprintf(pathname, UPGRADE_PATH, slash ? (slash + 1) : buf);
    if (!is_dir(pathname))
    {
        LOG_I("[%s]\"%s\" is not a directory", __FUNCTION__, pathname);
        *pathname = '\0';
        goto end;
    }

    cls_file(UPGRADE_FILE);

end:
    close(fd);
}

void list2vec(char *list, char **vec)
{
    if (list == NULL)
    {
        return;
    }

    char *token;
    int idx = 0;
    static const char* delim = "\t ";

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
                    printf("fopen(\"%s\") failed\n", cmdline_f);
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

    sprintf(time_s, "[%02d/%02d/%4d-%02d:%02d:%02d.%03ld]", tm1.tm_mon + 1, tm1.tm_mday, tm1.tm_year + 1900, tm1.tm_hour, tm1.tm_min, tm1.tm_sec, nanosec);
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
    char shell_cmd[256];
    pid_t pid  = -1;
    get_pid(&pid, name);
    if (pid != -1)
    {
        sprintf(shell_cmd, "kill -9 %d", pid);
        LOG_I("KILL \"%-7s\" by exec: \"%s\"", name, shell_cmd);
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
        LOG_I("cannot open \"%s\": %s", filepath, strerror(errno));
        return;
    }

    char *ret = fgets(str, len, fp);
    if (!ret)
    {
        LOG_I("read \"%s\" failed: %s", filepath, strerror(errno));
        return;
    }

    LOG_I("[%d][wchan] - %s\n", pid, str);
    if (wchan)
        strcpy(wchan, str);

    free(filepath);
    free(str);
    fclose(fp);
}

void exec_cmd(const char *shell_cmd)
{
    LOG_I("[THREAD-ID:%zu]exec: \"%s\"", pthread_self(), shell_cmd);
    system(shell_cmd);
}

int find_fd(pid_t pid, const char *lockfile)
{
    const char *path_fmt = "/proc/%d/fd";
    char pathname[64];
    sprintf(pathname, path_fmt, pid);
    struct dirent *file;
    DIR *dir;
#if EXT_LOG
    LOG_I("[%s]open directory \"%s\"", __FUNCTION__, pathname);
#endif
    dir = opendir(pathname);
    char filename[256];
    struct stat st;
    char realname[BUF_SIZE];
    int fd = -1;
    if (!dir)
    {
        LOG_I("cannot open \"%s\": %s", pathname, strerror(errno));
        return fd;
    }

    while((file = readdir(dir)))
    {
        if (strchr(file->d_name, '.'))
            continue;

        sprintf(filename, "%s/%s", pathname, file->d_name);
        lstat(filename, &st);
        if ((st.st_mode & S_IFMT) != S_IFLNK)
            continue;

        memset(realname, 0, BUF_SIZE);
        if (readlink(filename, realname, BUF_SIZE) == -1) /* no terminated null appended */
            continue;

#if EXT_LOG
        LOG_I("[%s]link file \"%s\"", __FUNCTION__, realname);
#endif
        if (strstr(realname, lockfile))
        {
            fd = atoi(file->d_name);
            goto end;
        }
    }

end:
    closedir(dir);
    return fd;
}

bool check_fd(const char *lockfile)
{
    int pid;
    LOG_I("[%s]check \"%s\"", __FUNCTION__, lockfile);
    if (access(lockfile, F_OK) != -1)
    {
        pid = -1;
        get_pid(&pid, "com.xiaomeng.icelocker");
        if (pid == -1)
        {
#if EXT_LOG
            LOG_I("[%s]IceLocker is dead...", __FUNCTION__);
#endif
            goto end;
        }

        int fd = find_fd(pid, lockfile);
        if (fd == -1)
        {
#if EXT_LOG
            LOG_I("cannot find find file descriptor");
            LOG_I("[%s]IceLocker is  D E A D", __FUNCTION__);
#endif
            goto end;
        }
        else
        {
#if EXT_LOG
            LOG_I("find fd %d of lockfile.txt", fd);
            LOG_I("[%s]IceLocker still  A L I V E", __FUNCTION__);
#endif
            return true;
        }
    }

end:
    return false;
}
