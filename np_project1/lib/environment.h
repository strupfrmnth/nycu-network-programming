#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

// #include "global.h"
#include <vector>
#include <string>
#include <sys/types.h>
#include "pipe.h"

using namespace std;

class Environment
{
private:
    const static int maxpipe = 1000;
public:
    int current_pos;
    Pipe pipes[maxpipe];
    Pipe last_pipe;
    vector<pid_t> child_processes[maxpipe];
    Environment();
    void print_env(vector<string> args);
    void set_env(vector<string> args);
    void exit_shell(vector<string> args);
    Pipe* getpipe(int pos);
    void setpipe(int pos, Pipe pipe);
    void enablepipe(int pos);
    void add_childprocess(int pos, pid_t pid);
    void add_childprocesses(int pos, vector<pid_t>& pids);
    vector<pid_t>& get_childprocess();
    void close_allpipes();
    void clear_env();
    ~Environment();
};

#endif