#include "parser.h"
#include "io.h"
#include <iostream>
#include <cstring>

Parser::Parser(string input) {
    this->input = input;
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
    while(idx < this->commands.size()) {
        if(idx == 0 || this->commands[idx-1] == "|") {
            Task task;
            task.args.push_back(this->commands[idx]);
            tasks.push_back(task);
            task_num++;
        } else if(this->commands[idx].compare("|") == 0) {
            //stdout to pipe
            tasks[task_num-1].io_stdout.type = IOType::ToPipe;
            tasks[task_num-1].io_stdout.pos = 0;
        } else if(this->commands[idx].compare(">") == 0) {
            //stdout to file
            tasks[task_num-1].io_stdout.type = IOType::ToFile;
            tasks[task_num-1].io_stdout.file = this->commands[idx+1];
            idx++;
        } else if(this->commands[idx][0] == '|' ) {
            string str_num = this->commands[idx].substr(1);
            int num = stoi(str_num);
            tasks[task_num-1].io_stdout.type = IOType::ToPipe;
            tasks[task_num-1].io_stdout.pos = num;
        } else if(this->commands[idx][0] == '!' ) {
            string str_num = this->commands[idx].substr(1);
            int num = stoi(str_num);
            tasks[task_num-1].io_stdout.type = IOType::ToPipe;
            tasks[task_num-1].io_stdout.pos = num;
            tasks[task_num-1].io_stderr.type = IOType::ToPipe;
            tasks[task_num-1].io_stderr.pos = num;
        } else{
            tasks[task_num-1].args.push_back(this->commands[idx]);
        }
        idx++;
    }
    return tasks;
}

Parser::~Parser() {
}