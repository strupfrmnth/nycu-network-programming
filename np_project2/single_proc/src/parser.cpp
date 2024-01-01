#include "parser.h"
#include "io.h"
#include <iostream>
#include <cstring>
#include <algorithm>

Parser::Parser(string input) {
    this->input = input;
}

string& Parser::ltrim(string& s) {
    s.erase(s.begin(),
            find_if(s.begin(), s.end(), [](int ch) { return !isspace(ch); }));
    return s;
}

vector<Task> Parser::parse() {
    char *dup = strdup(this->input.c_str());
    char *token = strtok(dup, " ");
    while(token != NULL) {
        this->commands.push_back(string(token));
        // cout << string(token) << endl;
        token = strtok(NULL, " ");
    }
    // command
    // command file
    // command file | 
    // command | command
    // command |num
    // command !num
    // command > file (no pipe)
    // command file > file
    unsigned idx = 0;
    int task_num = 0;
    vector<Task> tasks;
    if(this->commands[0] == "who") {
        Task task;
        task.args.push_back(this->commands[idx]);
        tasks.push_back(task);
        task_num++;
    } else if(this->commands[0] == "tell") {
        Task task;
        task.args.push_back(this->commands[idx]);
        tasks.push_back(task);
        task_num++;
        if(idx+1 < this->commands.size())
            tasks[task_num-1].args.push_back(this->commands[idx+1]);
        if(idx+2 < this->commands.size()) {
            int uid_str_pos = this->input.find(this->commands[idx+1]);
            int msg_str_pos = uid_str_pos + this->commands[idx+1].size();
            string msg = this->input.substr(msg_str_pos);
            msg = this->ltrim(msg);
            tasks[task_num-1].args.push_back(msg);
        }
        cout << "tell\n";
    } else if(this->commands[0] == "yell") {
        Task task;
        task.args.push_back(this->commands[idx]);
        tasks.push_back(task);
        task_num++;
        if(idx+1 < this->commands.size()) {
            int msg_str_pos = this->commands[0].size();
            string msg = this->input.substr(msg_str_pos);
            msg = this->ltrim(msg);
            tasks[task_num-1].args.push_back(msg);
        }
    } else if(this->commands[0] == "name") {
        Task task;
        task.args.push_back(this->commands[idx]);
        tasks.push_back(task);
        task_num++;
        cout << "name\n";
        if(idx+1 < this->commands.size()) {
            int name_str_pos = this->commands[0].size();
            string name = this->input.substr(name_str_pos);
            name = this->ltrim(name);
            tasks[task_num-1].args.push_back(name);
            cout << "in\n";
        }
    } else {
        while(idx < this->commands.size()) {
            if(idx == 0 || this->commands[idx-1] == "|") {
                Task task;
                task.args.push_back(this->commands[idx]);
                tasks.push_back(task);
                task_num++;
            } else if(this->commands[idx].compare("|") == 0) {
                //stdout to pipe
                tasks[task_num-1].io_stdout.type = IOType::ToDelayedPipe;
                tasks[task_num-1].io_stdout.pos = 0;
            } else if(this->commands[idx].compare(">") == 0) {
                //stdout to file
                tasks[task_num-1].io_stdout.type = IOType::ToFile;
                tasks[task_num-1].io_stdout.file = this->commands[idx+1];
                idx++;
            } else if(this->commands[idx][0] == '|') {
                string str_num = this->commands[idx].substr(1);
                int num = stoi(str_num);
                tasks[task_num-1].io_stdout.type = IOType::ToDelayedPipe;
                tasks[task_num-1].io_stdout.pos = num;
            } else if(this->commands[idx][0] == '!') {
                string str_num = this->commands[idx].substr(1);
                int num = stoi(str_num);
                tasks[task_num-1].io_stdout.type = IOType::ToDelayedPipe;
                tasks[task_num-1].io_stdout.pos = num;
                tasks[task_num-1].io_stderr.type = IOType::ToDelayedPipe;
                tasks[task_num-1].io_stderr.pos = num;
            } else if(this->commands[idx][0] == '>') {
                string str_num = this->commands[idx].substr(1);
                int num = stoi(str_num);
                tasks[task_num-1].io_stdout.type = IOType::ToUserPipe;
                tasks[task_num-1].io_stdout.uid = num;
                // tasks[task_num-1].io_stderr.type = IOType::ToUserPipe;
                // tasks[task_num-1].io_stderr.uid = num;
                if(idx+1 < this->commands.size()) {
                    if(this->commands[idx+1][0] == '<') {
                        string strnum = this->commands[idx+1].substr(1);
                        int n = stoi(strnum);
                        tasks[task_num-1].io_stdin.type = IOType::ToUserPipe;
                        tasks[task_num-1].io_stdin.uid = n;
                        idx++;
                    }
                }
            } else if(this->commands[idx][0] == '<') {
                string str_num = this->commands[idx].substr(1);
                int num = stoi(str_num);
                tasks[task_num-1].io_stdin.type = IOType::ToUserPipe;
                tasks[task_num-1].io_stdin.uid = num;
                if(idx+1 < this->commands.size()) {
                    if(this->commands[idx+1].compare(">") == 0) {
                        tasks[task_num-1].io_stdout.type = IOType::ToFile;
                        tasks[task_num-1].io_stdout.file = this->commands[idx+1];
                        idx += 2;
                    } else if(this->commands[idx+1][0] == '>') {
                        string strnum = this->commands[idx+1].substr(1);
                        int n = stoi(strnum);
                        tasks[task_num-1].io_stdout.type = IOType::ToUserPipe;
                        tasks[task_num-1].io_stdout.uid = n;
                        // tasks[task_num-1].io_stderr.type = IOType::ToUserPipe;
                        // tasks[task_num-1].io_stderr.uid = n;
                        idx++;
                    } else if(this->commands[idx+1].compare("|") == 0) {
                        tasks[task_num-1].io_stdout.type = IOType::ToDelayedPipe;
                        tasks[task_num-1].io_stdout.pos = 0;
                        idx++;
                    } else if(this->commands[idx+1][0] == '|') {
                        string strnum = this->commands[idx+1].substr(1);
                        int n = stoi(strnum);
                        tasks[task_num-1].io_stdout.type = IOType::ToDelayedPipe;
                        tasks[task_num-1].io_stdout.pos = n;
                        idx++;
                    } else if(this->commands[idx+1][0] == '!') {
                        string strnum = this->commands[idx+1].substr(1);
                        int n = stoi(strnum);
                        tasks[task_num-1].io_stdout.type = IOType::ToDelayedPipe;
                        tasks[task_num-1].io_stdout.pos = n;
                        tasks[task_num-1].io_stderr.type = IOType::ToDelayedPipe;
                        tasks[task_num-1].io_stderr.pos = n;
                        idx++;
                    }
                }
            } else{
                tasks[task_num-1].args.push_back(this->commands[idx]);
            }
            idx++;
        }
    }

    return tasks;
}

Parser::~Parser() {
}