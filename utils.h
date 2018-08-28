//
// Created by david on 7/3/18.
//

#ifndef C4FUN_UTILS_H
#define C4FUN_UTILS_H

#include <stdbool.h>
#include <signal.h>
#include <android/log.h>

#define      LOG_TAG "XIAOMENG.DAEMON"
#define   LOG_I(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define   LOG_E(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)


#define BUF_SIZE 256

typedef struct
{
    char name[256];
    bool is_alive;
    pid_t pid;
} proc_info;

void pr_exit(int state);
void check_apk(const char *path, char *apk);
void check_ver(char *pathname);
int link_data();
void list2vec(char *list, char **vec);
proc_info check_proc(const char *proc_name);
void get_pid(pid_t *pid, const char *proc_name);
void get_time(char *time_s);
size_t trim(char *out, const char *src);
void kill_proc(const char *name);
void lowerstr(char *str);
void getwchan(char *wchan, pid_t pid); /* waitting channel */
#endif //C4FUN_UTILS_H
