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

pid_t Task::exec(Shell& shell, int task_num, int task_size) {
    if(this->args[0] == "printenv") {
        shell.builtin_printenv(this->args);
    } else if(this->args[0] == "setenv") {
        shell.builtin_setenv(this->args);
    } else if(this->args[0] == "exit") {
        shell.builtin_exit();
    } else if(this->args[0] == "who") {
        shell.builtin_who();
    } else if(this->args[0] == "tell") {
        shell.builtin_tell(this->args);
    } else if(this->args[0] == "yell") {
        shell.builtin_yell(this->args);
    } else if(this->args[0] == "name") {
        shell.builtin_name(this->args);
    } else {
        this->pipe = shell.env.getpipe(0);
        if(!this->pipe->isenable()) {
            this->pipe->createp();
        } else {
            shell.env.last_pipe = *(this->pipe);
            Pipe new_pipe;
            shell.env.setpipe(0, new_pipe);
            this->pipe = shell.env.getpipe(0);
            this->pipe->createp();
        }
        if(this->io_stdout.type == IOType::ToDelayedPipe) {
            shell.env.enablepipe(this->io_stdout.pos);
        }
        if(this->io_stderr.type == IOType::ToDelayedPipe) {
            shell.env.enablepipe(this->io_stderr.pos);
        }

        pid_t pid = fork();
        if(pid < 0) {
            shell.env.getpipe(0)->closep();
            shell.env.setpipe(0, shell.env.last_pipe);
            return NPStatus::ForkError;
        } else if(pid > 0) {
            // parent process
            shell.env.last_pipe.closep();
            if(task_num == task_size) {
                this->pipe->closep();
            }
            this->pipe = nullptr;
            shell.env.add_childprocess(0, pid);
            return NPStatus::Success;
        } else {
            // child process
            // dup2(ssock, STDIN_FILENO);
            // if(this->io_stdin.type == IOType::Origin && env.last_pipe.isenable()) {
            //     env.last_pipe.dup2in(STDIN_FILENO);
            // }
            dup2(shell.ufd, STDIN_FILENO);
            dup2(shell.ufd, STDOUT_FILENO);
            dup2(shell.ufd, STDERR_FILENO);
            if(shell.env.last_pipe.isenable()) {
                shell.env.last_pipe.dup2in(STDIN_FILENO);
            }
            switch (this->io_stdout.type) {
                case IOType::Origin:
                    break;
                case IOType::ToDelayedPipe:
                    shell.env.getpipe(this->io_stdout.pos)->dup2out(STDOUT_FILENO);
                    break;
                case IOType::ToUserPipe:
                    // if(shell.server->id2usershell[this->io_stdout.uid]->env.user_pipes[shell.uid].isenable()) {
                    //     shell.server->id2usershell[this->io_stdout.uid]->env.user_pipes[shell.uid].dup2out(STDOUT_FILENO);
                    // }
                    if(shell.env.is_userpipe_out_error) {
                        int fd = open("/dev/null", O_WRONLY);
                        dup2(fd, STDOUT_FILENO);
                    } else if(shell.env.userpipefd != -1)
                    // kill(shell.env.userpipe_read_pid, SIGUSR2);
                    // int userpipefd = open(shell.env.fifo_name.c_str(), O_WRONLY);
                        dup2(shell.env.userpipefd, STDOUT_FILENO);
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
                case IOType::ToDelayedPipe:
                    shell.env.getpipe(this->io_stderr.pos)->dup2out(STDERR_FILENO);
                    break;
                case IOType::ToFile:
                    break;
                default:
                    break;
            }

            this->pipe->closep();
            shell.env.last_pipe.closep();
            shell.env.close_allpipes();
            close(shell.ufd);
            if(shell.env.userpipefd != -1) {
                close(shell.env.userpipefd);
                shell.env.userpipefd = -1;
            }
            const char* file = this->args[0].c_str();
            char** arg = new char*[this->args.size() + 1];
            for (size_t i = 0; i < this->args.size(); i++) {
                arg[i] = strdup(this->args[i].c_str());
            }
            arg[this->args.size()] = NULL;
            if(execvp(file, arg) < 0) {
                cerr << "Unknown command: [" << this->args[0] << "].\n";
                exit(0);
            }
            return NPStatus::Success;
        }
    }
    return NPStatus::Success;
}

Task::~Task() {
    
}