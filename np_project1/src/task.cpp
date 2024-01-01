#include "task.h"
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <cstring>
#include <iostream>

Task::Task() {
    this->pipe = nullptr;
}

pid_t Task::exec(Environment& env, int task_num, int task_size) {
    if(this->args[0] == "printenv") {
        env.print_env(this->args);
    } else if(this->args[0] == "setenv") {
        env.set_env(this->args);
    } else if(this->args[0] == "exit") {
        env.exit_shell(this->args);
    } else {
        this->pipe = env.getpipe(0);
        if(!this->pipe->isenable()) {
            this->pipe->createp();
        } else {
            env.last_pipe = *(this->pipe);
            Pipe new_pipe;
            env.setpipe(0, new_pipe);
            this->pipe = env.getpipe(0);
            this->pipe->createp();
        }
        if(this->io_stdout.type == IOType::ToPipe) {
            env.enablepipe(this->io_stdout.pos);
        }
        if(this->io_stderr.type == IOType::ToPipe) {
            env.enablepipe(this->io_stderr.pos);
        }

        pid_t pid = fork();
        if(pid < 0) {
            env.getpipe(0)->closep();
            env.setpipe(0, env.last_pipe);
            return NPStatus::ForkError;
        } else if(pid > 0) {
            // parent process
            env.last_pipe.closep();
            if(task_num == task_size) {
                this->pipe->closep();
            }
            this->pipe = nullptr;
            env.add_childprocess(0, pid);
            return NPStatus::Success;
        } else {
            // child process
            if(env.last_pipe.isenable()) {
                env.last_pipe.dup2in(STDIN_FILENO);
            }
            switch (this->io_stdout.type) {
                case IOType::Origin:
                    break;
                case IOType::ToPipe:
                    env.getpipe(this->io_stdout.pos)->dup2out(STDOUT_FILENO);
                    break;
                case IOType::ToFile:
                    {int fd = open(this->io_stdout.file.c_str(),
                                (O_WRONLY | O_CREAT | O_TRUNC),
                                (S_IRUSR | S_IWUSR));
                    dup2(fd, STDOUT_FILENO);}
                    break;
                default:
                    break;
            }
            switch (this->io_stderr.type) {
                case IOType::Origin:
                    break;
                case IOType::ToPipe:
                    env.getpipe(this->io_stderr.pos)->dup2out(STDERR_FILENO);
                    break;
                case IOType::ToFile:
                    break;
                default:
                    break;
            }

            this->pipe->closep();
            env.last_pipe.closep();
            env.close_allpipes();

            const char* file = this->args[0].c_str();
            char** arg = new char*[this->args.size() + 1];
            for (size_t i = 0; i < this->args.size(); i++) {
                arg[i] = strdup(this->args[i].c_str());
            }
            arg[this->args.size()] = NULL;
            if(execvp(file, arg) < 0) {
                cerr << "Unknown command: [" << this->args[0] << "]." << endl;
                exit(0);
            }
            return NPStatus::Success;
        }
    }
    return NPStatus::Success;
}

Task::~Task() {
    
}