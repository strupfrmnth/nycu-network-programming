#include "environment.h"
#include <iostream>
#include <cstdlib>
#include <signal.h>

Environment::Environment()
{
    this->current_pos = 0;
    this->is_userpipe_out_error = false;
    this->is_userpipe_in_error = false;
}

void Environment::print_env(vector<string> args) {
    if(args.size() < 2) {
        cerr << "Invalid arguments" << endl;
    } else {
        const char* envp = getenv(args[1].c_str());
        if (envp) {
            cout << getenv(args[1].c_str()) << endl;
        } else {
            // cout << endl;
        }
    }
}

void Environment::set_env(vector<string> args) {
    if (args.size() < 3) {
        cerr << "Invalid arguments" << endl;
    } else {
        this->env_variables[args[1]] = args[2];
        setenv(args[1].c_str(), args[2].c_str(), 1);
    }
}

void Environment::exit_shell(vector<string> args) {
    for(int i = 0; i < this->maxpipe; i++) {
        for(auto pid : this->child_processes[i]) {
            kill(pid, SIGKILL);
        }
    }
    exit(0);
}

Pipe* Environment::getpipe(int pos) {
    int real_pos = (pos + this->current_pos) % maxpipe;
    return &(this->pipes[real_pos]);
}

void Environment::setpipe(int pos, Pipe pipe) {
    int real_pos = (pos + this->current_pos) % maxpipe;
    this->pipes[real_pos] = pipe;
}

void Environment::enablepipe(int pos) {
    int real_pos = (pos + this->current_pos) % maxpipe;
    if(!this->pipes[real_pos].isenable())
        this->pipes[real_pos].createp();
}

Pipe* Environment::getuserpipe(int uid) {
    if(this->user_pipes.find(uid) != this->user_pipes.end())
        return &(this->user_pipes[uid]);
    else
        return nullptr;
}

void Environment::add_childprocess(int pos, pid_t pid) {
    int real_pos = (pos + this->current_pos) % maxpipe;
    this->child_processes[real_pos].push_back(pid);
}

void Environment::add_childprocesses(int pos, vector<pid_t>& pids) {
    int real_pos = (pos + this->current_pos) % maxpipe;
    this->child_processes[real_pos].insert(
        this->child_processes[real_pos].end(), pids.begin(), pids.end());
    pids.clear();
}

vector<pid_t>& Environment::get_childprocess(int pos) {
    int real_pos = (pos + this->current_pos) % maxpipe;
    return this->child_processes[real_pos];
}

vector<pid_t>& Environment::get_userchildprocess(int uid) {
    return this->user_child_processes[uid];
}

void Environment::move2_child_processes(int pos, vector<pid_t>& pids) {
    int real_pos = (pos + this->current_pos) % maxpipe;
    this->child_processes[real_pos].insert(this->child_processes[real_pos].end(), pids.begin(), pids.end());
    pids.clear();
}

void Environment::move2_user_child_processes(int uid, vector<pid_t>& pids) {
    this->user_child_processes[uid].insert(this->user_child_processes[uid].end(), pids.begin(), pids.end());
    pids.clear();
}

void Environment::close_allpipes() {
    for (int i = 0; i < maxpipe; i++) {
        this->pipes[i].closep();
    }
}

void Environment::clear_env() {
    this->pipes[this->current_pos].closep();
    this->child_processes[this->current_pos].clear();
    this->current_pos = (this->current_pos + 1) % maxpipe;
}

Environment::~Environment()
{
}