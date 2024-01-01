#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

// #include "global.h"
#include <vector>
#include <string>
#include <map>
#include <sys/types.h>
#include "pipe.h"

using namespace std;

class Environment
{
private:
    const static int maxpipe = 1500;
public:
    int current_pos;
    Pipe pipes[maxpipe];
    map<int, Pipe> user_pipes;
    bool is_userpipe_out_error;
    bool is_userpipe_in_error;
    Pipe last_pipe;
    vector<pid_t> child_processes[maxpipe];
    map<int, vector<pid_t> > user_child_processes;
    map<string, string> env_variables;
    Environment();
    void print_env(vector<string> args);
    void set_env(vector<string> args);
    void exit_shell(vector<string> args);
    Pipe* getpipe(int pos);
    void setpipe(int pos, Pipe pipe);
    void enablepipe(int pos);
    Pipe* getuserpipe(int uid);
    void add_childprocess(int pos, pid_t pid);
    void add_childprocesses(int pos, vector<pid_t>& pids);
    vector<pid_t>& get_childprocess(int pos);
    vector<pid_t>& get_userchildprocess(int pos);
    void move2_child_processes(int pos, vector<pid_t>& pids);
    void move2_user_child_processes(int uid, vector<pid_t>& pids);
    void close_allpipes();
    void clear_env();
    ~Environment();
};

#endif