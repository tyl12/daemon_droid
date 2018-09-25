
#include <stdbool.h>
#include <signal.h>
#include <sys/file.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <libgen.h>
#include <iostream>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <string>
#include <thread>
#include <vector>
#include <fstream>
#include <vector>
#include <dirent.h>
#include <dirent.h>
#include <algorithm>

#if defined(__ANDROID__)
#include <android/log.h>
#endif

using namespace std;

#if defined(__ANDROID__)
#define   LOG_TAG "xiaomengDaemon"
#if 0
#define   LOG_I(...) do{ __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__); printf(__VA_ARGS__); } while(0)
#define   LOG_E(...) do{ __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__); printf(__VA_ARGS__); } while(0)
#else
#define   LOG_I(...) do{ __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__); } while(0)
#define   LOG_E(...) do{ __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__); } while(0)
#endif

#else
#define   LOG_I(fmt, ...) printf("xiaomengDaemon:" fmt "\n", ##__VA_ARGS__)
#define   LOG_E(fmt, ...) printf("xiaomengDaemon:" fmt "\n", ##__VA_ARGS__)
#endif

int startsWith(string s, string sub){
        return s.find(sub)==0?1:0;
}

int endsWith(string s,string sub){
        return s.rfind(sub)==(s.length()-sub.length())?1:0;
}

int exec_cmd(const char *shell_cmd)
{
    LOG_I("[THREAD-ID:%zu]exec: \"%s\"\n", pthread_self(), shell_cmd);
    pid_t status = system(shell_cmd);
    if (-1 == status) {
        LOG_I("system error!");
        return -1;
    }
    LOG_I("exit status value = [0x%x]\n", status);
    LOG_I("exit status = [%d]\n", WEXITSTATUS(status));
    if (WIFEXITED(status)) {
        if (0 == WEXITSTATUS(status)) {
            LOG_I("run system call <%s> successfully.\n", shell_cmd);
        }
        else {
            LOG_I("run system call <%s> fail, script exit code: %d\n", shell_cmd, WEXITSTATUS(status));
        }
    }
    return WEXITSTATUS(status);
}

#define BUF_LEN (256)
int exec_popen(const char* cmd) {

    char buffer[BUF_LEN];
    std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
    if (!pipe){
        LOG_I("ERROR! fail to popen %s\n", cmd);
        throw std::runtime_error("popen() failed!");
    }
    while (!feof(pipe.get())) {
        if (fgets(buffer, BUF_LEN, pipe.get()) != nullptr)
            LOG_I("%s",buffer);
    }
    return 0;
}

int wait_system_boot_complete(){
    printf("%s: start to check mount list\n",__FUNCTION__);
    const vector<string> mountlist={
        "/data",
        "/mnt/runtime/default/emulated",
        "/storage/emulated",
        "/mnt/runtime/read/emulated",
        "/mnt/runtime/write/emulated"
    };
    for (const auto& mnt:mountlist){
        printf("%s: check for mount point: %s", __FUNCTION__, mnt.c_str());
        while(true){
            string cmd="mountpoint -q " + mnt;
            printf("%s: execute cmd: %s\n", __FUNCTION__, cmd.c_str());
            if (exec_cmd(cmd.c_str()) == 0){
                printf("%s: %s is mounted", __FUNCTION__, mnt.c_str());
                break; //continue to next mnt point
            }
            else{
                printf("%s: %s is NOT mounted, wait", __FUNCTION__, mnt.c_str());
                sleep(10);
                continue;
            }
        }
    }
    sleep(10);
    return 0;
}

int main(){
    //wait for /sdcard, /data mounted
    sleep(60);

    wait_system_boot_complete();

#if 0
    string cmd = "logwrapper /system/bin/xiaomengDaemon_internal";
    LOG_I("start to execute: %s\n", cmd.c_str());
    int ret = exec_cmd(cmd.c_str());
#else
    string cmd = "/system/bin/xiaomengDaemon_internal 2>&1";
    LOG_I("start to execute: %s\n", cmd.c_str());
    int ret = exec_popen(cmd.c_str());
#endif
    if (ret){
        LOG_I("ERROR: execute %s failed. retry\n", cmd.c_str());
    }
    LOG_I("ERROR! xiaomeng daemon exit!\n");
    return 0;
}

