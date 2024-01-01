#ifndef TASK_H
#define TASK_H

#include <vector>
#include <string>
#include "environment.h"
#include "io.h"
#include "pipe.h"
#include "server_and_shell.h"

using namespace std;

enum NPStatus {
    Success = 0,
    ForkError = -1,
    FileNotFound = -2
};

class Task
{
private:
    
public:
    vector<string> args;
    IO io_stdin;
    IO io_stdout;
    IO io_stderr;
    Pipe* pipe;
    Task();
    pid_t exec(Shell& shell, int task_num, int task_size);
    ~Task();
};

#endif