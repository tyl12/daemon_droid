
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

#if 1
#define SHELL_BIN               "sh"
#define SCRIPT_ENV              "/data/local/tmp/.environment"
#else //for debug only
#define SHELL_BIN               "bash"
#define SCRIPT_ENV              "./sdcard/.environment"
#endif

#if 0//defined(__ANDROID__) //build as internal, with logwrapper
#define   LOG_TAG "deepDaemon"
#define   LOG_I(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define   LOG_E(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#else
#define   LOG_I(...) printf(__VA_ARGS__)
#define   LOG_E(...) printf(__VA_ARGS__)
#endif

int startsWith(string s, string sub){
    return s.find(sub)==0?1:0;
}

int endsWith(string s,string sub){
    return s.rfind(sub)==(s.length()-sub.length())?1:0;
}

vector<string> get_file_list(const char* filedir, const char* str_start=NULL, const char* str_end=NULL)
{
    vector<string> result;
    struct dirent *ptr;
    DIR *dir;
    dir=opendir(filedir);

    LOG_I("filelist:\n");
    while((ptr=readdir(dir))!=NULL)
    {
        //跳过'.'和'..'两个目录
        if((strlen(ptr->d_name) == 1 &&  ptr->d_name[0] == '.'))
            continue;

        if((strlen(ptr->d_name) == 2 &&  ptr->d_name[0] == '.' && ptr->d_name[1] == '.' ))
            continue;

        if (str_start != NULL && !startsWith(ptr->d_name, str_start))
            continue;

        if (str_end != NULL && !endsWith(ptr->d_name, str_end))
            continue;

        LOG_I("%s\n",ptr->d_name);
        result.push_back(string(ptr->d_name));
    }
    closedir(dir);

    sort(result.begin(), result.end());

    LOG_I("sorted file list:\n");
    int i = 0;
    for (auto s:result){
        LOG_I("%02d:%s\n", i, s.c_str());
        i++;
    }
    LOG_I("\n");
    return result;
}


int exec_cmd(const char *shell_cmd)
{
    LOG_I("%s: thread-id=%zu, cmd: \"%s\"\n", __FUNCTION__, pthread_self(), shell_cmd);
    pid_t status = system(shell_cmd);
    if (-1 == status) {
        LOG_I("%s:ERROR: system error!\n", __FUNCTION__);
        return -1;
    }
    LOG_I("%s: exit status value = [0x%x]\n", __FUNCTION__, status);
    LOG_I("%s: exit status = [%d]\n", __FUNCTION__, WEXITSTATUS(status));
    if (WIFEXITED(status)) {
        if (0 == WEXITSTATUS(status)) {
            LOG_I("%s: run system call <%s> successfully.\n", __FUNCTION__, shell_cmd);
        }
        else {
            LOG_I("%s: ERROR: run system call <%s> fail, script exit code: [%d]\n", __FUNCTION__, shell_cmd, WEXITSTATUS(status));
        }
    }
    return WEXITSTATUS(status);
}

const string WHITE_SPACE_STR=" \n\r\t\f\v";
string ltrim(const string& s)
{
    size_t start = s.find_first_not_of(WHITE_SPACE_STR);
    return (start == string::npos)? "" : s.substr(start);
}

int load_env(){
    ifstream in(SCRIPT_ENV);
    string line_in;
    string line;
    while(in >> line_in){
        LOG_I("readline: %s\n", line_in.c_str());
        line=ltrim(line_in);
        if (line[0] == '#'){
            LOG_I("skip comment line:%s\n", line_in.c_str());
            continue;
        }
        size_t pos = line.find_first_of("=", 0);
        if (pos == 0 || pos == line.size() || pos == string::npos){
            LOG_I("invalid line: %s\n", line.c_str());
            continue;
        }
        string key=line.substr(0,pos-0);
        string val=line.substr(pos+1);
        if (key.find_first_of(".", 0) != string::npos){
            LOG_I("invalid key: %s\n", key.c_str());
            continue;
        }

        setenv(key.c_str(), val.c_str(), 1);
        LOG_I("-> setenv: %s=%s\n", key.c_str(), val.c_str());
        //LOG_I("|%s|\n",getenv(key.c_str()));
    }

    return 0;
}

int wait_system_boot_complete(){
    LOG_I("%s: start to check mount list\n",__FUNCTION__);
    const vector<string> mountlist={
        "/data",
        "/mnt/runtime/default/emulated",
        "/storage/emulated",
        "/mnt/runtime/read/emulated",
        "/mnt/runtime/write/emulated"
    };
    for (const auto& mnt:mountlist){
        LOG_I("%s: check for mount point: %s", __FUNCTION__, mnt.c_str());
        while(true){
            string cmd="mountpoint -q " + mnt;
            LOG_I("%s: execute cmd: %s\n", __FUNCTION__, cmd.c_str());
            if (exec_cmd(cmd.c_str()) == 0){
                LOG_I("%s: %s is mounted", __FUNCTION__, mnt.c_str());
                break; //continue to next mnt point
            }
            else{
                LOG_I("%s: %s is NOT mounted, wait", __FUNCTION__, mnt.c_str());
                sleep(10);
                continue;
            }
        }
    }
    sleep(10);
    return 0;
}


int main(){

    sleep(10);

    //wait for /sdcard, /data mounted
    wait_system_boot_complete();

    load_env();

    const char* SCRIPT_DIR=getenv("ENV_DEEPLEARN_SCRIPT_DIR");
    if (SCRIPT_DIR==NULL){
        SCRIPT_DIR="/data/local/tmp";
        setenv("ENV_DEEPLEARN_SCRIPT_DIR", SCRIPT_DIR, 1);
    }
    setenv("ANDROID_DATA", "/data", 1);
    setenv("ANDROID_ROOT", "/system", 1);

    LOG_I("ENV_DEEPLEARN_SCRIPT_DIR is: %s\n", SCRIPT_DIR);

    vector<string> init_list = get_file_list(SCRIPT_DIR, "init", ".sh");
    vector<string> once_list = get_file_list(SCRIPT_DIR, "once", ".sh");
    vector<string> period_list = get_file_list(SCRIPT_DIR, "period", ".sh");

    thread t_init = thread([=](){
        int ret = 0;
        for (auto f:init_list){
            while(true){
                string cmd = string(SHELL_BIN) + " " + SCRIPT_DIR + "/" + f;
                LOG_I("start to execute: %s\n", f.c_str());
                cmd += " 2>&1 | sed  's/^/["+f+"] /'";
                ret = exec_cmd(cmd.c_str());
                if (ret){
                    LOG_I("ERROR: execute %s failed. retry\n", f.c_str());
                    sleep(10);
                    continue;
                }
                LOG_I("execute %s success. continue\n", f.c_str());
                break;
            }
        }
    });
    if (t_init.joinable()){ t_init.join(); }
    LOG_I("execute all <init> scripts complete\n");


    thread t_once = thread([=](){
        int ret = 0;
        for (auto f:once_list){
            while(true){
                string cmd = string(SHELL_BIN) + " " + SCRIPT_DIR + "/" + f;
                LOG_I("start to execute: %s\n", f.c_str());
                cmd += " 2>&1 | sed  's/^/["+f+"] /'";
                ret = exec_cmd(cmd.c_str());
                if (ret){
                    LOG_I("ERROR: execute %s failed. retry\n", f.c_str());
                    sleep(10);
                    continue;
                }
                LOG_I("execute %s success. continue\n", f.c_str());
                break;
            }
        }
    });

    vector<thread> period_threads;
    for (auto f:period_list){
        period_threads.push_back(
            thread([=](){
                int ret=0;
                while(true){
                    string cmd = string(SHELL_BIN) + " " + SCRIPT_DIR + "/" + f;
                    LOG_I("start to execute: %s\n", f.c_str());
                    cmd += " 2>&1 | sed  's/^/["+f+"] /'";
                    ret = exec_cmd(cmd.c_str());
                    if (ret){
                        LOG_I("ERROR: execute %s failed. retry\n", f.c_str());
                        sleep(10);
                        continue;
                    }
                    LOG_I("execute %s success. next loop\n", f.c_str());
                    sleep(10);
                }
            })
        );
    }

    if (t_once.joinable()){ t_once.join(); }
    LOG_I("execute all <once> scripts complete\n");

    for (auto && t_period:period_threads){
        if (t_period.joinable()){ t_period.join(); }
    }
    LOG_I("execute all <period> scripts complete\n");

    //to avoid no script available
    do{
        LOG_I("ERROR: deeplearn daemon run in IDLE state...\n");
        sleep(60);
    } while(1);

    LOG_I("ERROR! deeplearn daemon exit!\n");

    return 0;
}

