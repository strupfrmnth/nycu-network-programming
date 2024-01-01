#include "server_and_shell.h"
#include "parser.h"
#include "task.h"
#include "pipe.h"
#include <unistd.h>
#include <iostream>
#include <memory.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

template <class T>
ssize_t Server::safe_send(int fd, const T& v) {
    const size_t send_buffersize = 512;
    for (size_t i = 0; i < v.size();) {
        size_t t = send(fd, &v[0] + i, min(send_buffersize, v.size()-i), 0);
        if (t != size_t(-1)) {
            i += t;
        } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            ;
        } else if (errno == EINTR) {
            ;
        } else {
            return ssize_t(-1);
        }
    }
    return v.size();
}

Server::Server(int msock) : id2usershell({nullptr}) {
    this->msock = msock;
    this->maxfd = msock;
    FD_ZERO(&(this->afds));
    FD_SET(msock, &(this->afds));
}

int Server::create_id() {
    int uid = -1;
    if (this->available_ids.empty()) {
        uid = this->id2usershell.size();
        this->id2usershell.push_back(nullptr);
    } else {
        uid = this->available_ids.top();
        this->available_ids.pop();
    }
    return uid;
}

shared_ptr<Shell> Server::create_shell(sockaddr_in* client_info, int ufd) {
    int uid = this->create_id();
    shared_ptr<Shell> shell_ptr(new Shell(client_info, ufd, uid, *this));
    if (this->id2usershell[uid] != nullptr || this->fd2usershell[ufd] != nullptr) {
        cerr << "Failed to create user: uid or ufd is occupied!!!" << endl;
        exit(1);
    }
    this->id2usershell[uid] = shell_ptr;
    this->fd2usershell[ufd] = shell_ptr;
    this->id2username[uid] = "(no name)";
    return shell_ptr;
}

vector<weak_ptr<Shell>> Server::getusers(){
    vector<weak_ptr<Shell>> users(this->id2usershell.size());
    for (auto& user : this->id2usershell) {
        users.push_back(weak_ptr<Shell>(user));
    }
    return users;
}

void Server::deleteuser(Shell& shell) {
    int uid = shell.uid;
    int ufd = shell.ufd;
    auto shell_ptr = this->id2usershell[uid];
    if (shell_ptr != nullptr && shell_ptr == this->fd2usershell[ufd]) {
        this->garbage_shells.push(shell_ptr);
    } else {
        cerr << "Failed to add user to garbage queue!!!\n";
    }
}

weak_ptr<Shell> Server::getuserbyid(int uid) {
    return this->id2usershell[uid];
}

void Server::clear_garbage_shells() {
    while(!this->garbage_shells.empty()) {
        auto shell = this->garbage_shells.front();
        this->garbage_shells.pop();
        if(shell.use_count() > 3) {
            cerr << "Failed to delete user: shell's shared_ptr is owned by others\n";
        } else {
            string msg = "*** User '" + shell->name + "' left. ***\n";
            this->broadcast(msg);
            for(auto u_w : this->getusers()) {
                if(auto u = u_w.lock()) {
                    for(auto& pid : u->env.get_userchildprocess(shell->uid)) {
                        kill(pid, SIGKILL);
                    }
                    u->env.get_userchildprocess(shell->uid).clear();
                    if(u->env.getuserpipe(shell->uid) != nullptr) {
                        u->env.getuserpipe(shell->uid)->closep();
                        u->env.user_pipes.erase(shell->uid);
                    }
                }
            }
            this->id2usershell[shell->uid].reset();
            this->fd2usershell[shell->ufd].reset();
            FD_CLR(shell->ufd, &(this->afds));
            this->id2username.erase(shell->uid);
            this->available_ids.push(shell->uid);
            if(this->maxfd == shell->ufd) {
                this->maxfd--;
            }
        }
    }
}

ssize_t Server::broadcast(const string& msg) {
    ssize_t rtn = 0;
    for (auto& fd_user : this->fd2usershell) {
        int ufd = fd_user.first;
        rtn |= this->safe_send(ufd, msg);
    }
    return rtn;
}

ssize_t Server::send_welcomemsg(int fd) {
    static string msg(
        "****************************************\n"
        "** Welcome to the information server. **\n"
        "****************************************\n"
    );
    return this->safe_send(fd, msg);
}

ssize_t Server::send_percentmsg(int fd) {
    static string msg("% ");
    return this->safe_send(fd, msg);
}

bool Server::user_exists(int uid) {
    if(uid < int(this->id2usershell.size())) {
        if(this->id2usershell[uid] != nullptr) {
            return true;
        }
        return false;
    }
    return false;
}

void Server::run() {
    int max_buffer_size = 15001;
    fd_set rfds;
    while(true) {
        this->clear_garbage_shells();
        rfds = this->afds;
        if (select(this->maxfd + 1, &rfds, NULL, NULL, NULL) == -1) {
            continue;
        }
        struct sockaddr_in client_info;
        socklen_t addrlen = sizeof(client_info);
        for(int fd = 0; fd <= this->maxfd; fd++) {
            if(FD_ISSET(fd, &rfds)) {
                if(fd == this->msock) {
                    int ssock = accept(fd, (struct sockaddr *)&client_info, &addrlen);
                    auto shell = this->create_shell(&client_info, ssock);
                    FD_SET(ssock, &(this->afds));
                    if(ssock > this->maxfd) {
                        this->maxfd = ssock;
                    }
                    this->send_welcomemsg(ssock);
                    this->broadcast("*** User '(no name)' entered from " + shell->addr + ":" + shell->port + ". ***\n");
                    this->send_percentmsg(ssock);
                    cout << "User connected" << endl;
                } else {
                    auto shell = this->fd2usershell[fd];
                    if(shell == nullptr) {
                        cerr << "shell with given sockfd not exists!!!" << endl;
                        exit(1);
                    } else if(!shell->isalive) {
                        continue;
                    }
                    char buffer[max_buffer_size];
                    memset(&buffer, '\0', sizeof(buffer));
                    if(recv(fd, buffer, sizeof(buffer), 0) <= 0) {
                        shell->execute("exit");
                    } else {
                        shell->execute(string(buffer));
                        if(shell->isalive) {
                            this->send_percentmsg(fd);
                        }
                    }
                }
            }
        }
    }
    
}

Server::~Server()
{
}

Shell::Shell(sockaddr_in* client_info, int ufd, int uid, Server& server)
 : server(server) {
    this->ufd = ufd;
    this->uid = uid;
    this->isalive = true;
    string str(inet_ntoa(client_info->sin_addr));
    this->addr = str;
    this->port = to_string(ntohs(client_info->sin_port));
    this->name = "(no name)";
    this->env.set_env({"setenv", "PATH", "bin:."});
}

void Shell::builtin_exit() {
    this->server.deleteuser(*this);
    this->isalive = false;
}

void Shell::builtin_printenv(vector<string> args) {
    auto inherit_stdout = dup(STDOUT_FILENO);
    auto inherit_stderr = dup(STDERR_FILENO);
    dup2(this->ufd, STDOUT_FILENO);
    dup2(this->ufd, STDERR_FILENO);
    this->env.print_env(args);
    dup2(inherit_stdout, STDOUT_FILENO);
    dup2(inherit_stderr, STDERR_FILENO);
    close(inherit_stdout);
    close(inherit_stderr);
}

void Shell::builtin_setenv(vector<string> args) {
    auto inherit_stdout = dup(STDOUT_FILENO);
    auto inherit_stderr = dup(STDERR_FILENO);
    dup2(this->ufd, STDOUT_FILENO);
    dup2(this->ufd, STDERR_FILENO);
    this->env.set_env(args);
    dup2(inherit_stdout, STDOUT_FILENO);
    dup2(inherit_stderr, STDERR_FILENO);
    close(inherit_stdout);
    close(inherit_stderr);
}

void Shell::builtin_who() {
    auto inherit_stdout = dup(STDOUT_FILENO);
    auto inherit_stderr = dup(STDERR_FILENO);
    dup2(this->ufd, STDOUT_FILENO);
    dup2(this->ufd, STDERR_FILENO);
    
    cout << "<ID>\t<nickname>\t<IP:port>\t<indicate me>\n";
    for (weak_ptr<Shell>& user_w : this->server.getusers()) {
        auto user = user_w.lock();
        if (user) {
            cout << user->uid << "\t" << user->name << "\t";
            cout << user->addr + ":" + user->port;
            if(this->uid == user->uid) {
                cout << "\t<-me\n";
            } else {
                cout << "\n";
            }
        }
    }

    dup2(inherit_stdout, STDOUT_FILENO);
    dup2(inherit_stderr, STDERR_FILENO);
    close(inherit_stdout);
    close(inherit_stderr);
}

void Shell::builtin_tell(vector<string> args) {
    auto inherit_stdout = dup(STDOUT_FILENO);
    auto inherit_stderr = dup(STDERR_FILENO);
    dup2(this->ufd, STDOUT_FILENO);
    dup2(this->ufd, STDERR_FILENO);

    if(args.size() < 3) {
        cerr << "Invalid arguments" << endl;
    } else {
        string uid_to_str = args[1];
        string msg = args[2];
        int uid_to = stoi(uid_to_str);
        if(this->server.user_exists(uid_to)) {
            auto user_to = this->server.getuserbyid(uid_to).lock();
            string message = "*** " + this->name + " told you ***: " + msg + "\n";
            user_to->server.safe_send(user_to->ufd, message);
        } else {
            cout << "*** Error: user #" << uid_to << " does not exist yet. ***\n";
        }
    }
    
    dup2(inherit_stdout, STDOUT_FILENO);
    dup2(inherit_stderr, STDERR_FILENO);
    close(inherit_stdout);
    close(inherit_stderr);
}

void Shell::builtin_yell(vector<string> args) {
    auto inherit_stdout = dup(STDOUT_FILENO);
    auto inherit_stderr = dup(STDERR_FILENO);
    dup2(this->ufd, STDOUT_FILENO);
    dup2(this->ufd, STDERR_FILENO);

    if(args.size() < 2) {
        cerr << "Invalid arguments" << endl;
    } else {
        string msg = args[1];
        string yell_msg = "*** " + this->name + " yelled ***: " + msg + "\n";
        this->server.broadcast(yell_msg);
    }

    dup2(inherit_stdout, STDOUT_FILENO);
    dup2(inherit_stderr, STDERR_FILENO);
    close(inherit_stdout);
    close(inherit_stderr);
}

void Shell::builtin_name(vector<string> args) {
    auto inherit_stdout = dup(STDOUT_FILENO);
    auto inherit_stderr = dup(STDERR_FILENO);
    dup2(this->ufd, STDOUT_FILENO);
    dup2(this->ufd, STDERR_FILENO);

    if(args.size() < 2) {
        cerr << "Invalid arguments" << endl;
    } else {
        string new_name = args[1];
        bool issame = false;
        for(auto it = this->server.id2username.begin(); it != this->server.id2username.end(); ++it) {
            if(it->second.compare(new_name) == 0) {
                issame = true;
                break;
            }
        }
        if(issame) {
            cout << "*** User '" + new_name + "' already exists. ***\n";
        } else {
            string msg = "*** User from " + this->addr + ":" + this->port;
            msg += " is named '" + new_name + "'. ***\n";
            this->server.broadcast(msg);
            this->server.id2username[this->uid] = new_name;
            this->name = new_name;
        }
    }

    dup2(inherit_stdout, STDOUT_FILENO);
    dup2(inherit_stderr, STDERR_FILENO);
    close(inherit_stdout);
    close(inherit_stderr);
}

int Shell::wait_child_processes() {
    int status;
    for (auto& pid : this->env.get_childprocess(0)) {
        waitpid(pid, &status, 0);
    }
    this->env.get_childprocess(0).clear();
    return status;
}

int Shell::wait_user_child_processes(int uid) {
    int status;
    for (auto& pid : this->env.get_userchildprocess(uid)) {
        waitpid(pid, &status, 0);
    }
    this->env.get_userchildprocess(uid).clear();
    return status;
}

bool Shell::move_child_processes2user(int uid, vector<pid_t>& pids) {
    if(this->server.user_exists(uid)) {
        this->server.id2usershell[uid]->env.move2_user_child_processes(this->uid, pids);
        return true;
    }
    return false;
}

void Shell::execute(string input) {
    clearenv();
    for(auto& vars : this->env.env_variables) {
        setenv(vars.first.c_str(), vars.second.c_str(), 1);
    }

    std::size_t found = input.find("\n");
    if (found!=std::string::npos)
        input.pop_back();
    found = input.find("\r");
    if (found!=std::string::npos)
        input.pop_back();
    Parser parser(input);
    vector<Task> tasks = parser.parse();
    if(tasks.empty()) {
        return;
    }

    auto first_task = tasks.begin();
    auto last_task = tasks.rbegin();
    if(first_task->io_stdin.type == IOType::ToUserPipe) {
        int uid_from = first_task->io_stdin.uid;
        if(!this->server.user_exists(uid_from)) {
            string msg = "*** Error: user #" + to_string(uid_from) + " does not exist yet. ***\n";
            this->server.safe_send(this->ufd, msg);
            Pipe fake_pipe;
            fake_pipe.pfd[fake_pipe.in] = open("/dev/null", O_RDWR);
            this->env.last_pipe = fake_pipe;
            this->env.is_userpipe_in_error = true;
            // this->wait_child_processes();
            // return;
        } else if(this->env.getuserpipe(uid_from) != nullptr) {
            if(this->env.getuserpipe(uid_from)->isenable()) {
                string msg = "*** " + this->name + " (#" + to_string(this->uid);
                msg += ") just received from " + this->server.id2usershell[uid_from]->name;
                msg += " (#" + to_string(uid_from) + ") by '" + input + "' ***\n";
                this->server.broadcast(msg);
                this->env.last_pipe = *(this->env.getuserpipe(uid_from));
                Pipe new_pipe;
                this->env.user_pipes[uid_from] = new_pipe;
            } else {
                string msg = "*** Error: the pipe #" + to_string(uid_from) + "->#" + to_string(this->uid) + " does not exist yet. ***\n";
                this->server.safe_send(this->ufd, msg);
                Pipe fake_pipe;
                fake_pipe.pfd[fake_pipe.in] = open("/dev/null", O_RDWR);
                this->env.last_pipe = fake_pipe;
                this->env.is_userpipe_in_error = true;
            }
        } else {
            string msg = "*** Error: the pipe #" + to_string(uid_from) + "->#" + to_string(this->uid) + " does not exist yet. ***\n";
            this->server.safe_send(this->ufd, msg);
            Pipe fake_pipe;
            fake_pipe.pfd[fake_pipe.in] = open("/dev/null", O_RDWR);
            this->env.last_pipe = fake_pipe;
            this->env.is_userpipe_in_error = true;
            // this->wait_child_processes();
            // return;
        }
    }

    if(last_task->io_stdout.type == IOType::ToUserPipe) {
        int uid_to = last_task->io_stdout.uid;
        if(!this->server.user_exists(uid_to)) {
            string msg = "*** Error: user #" + to_string(uid_to) + " does not exist yet. ***\n";
            this->server.safe_send(this->ufd, msg);
            // if (first_task->io_stdin.type == IOType::ToUserPipe) {
            //     this->wait_user_child_processes(first_task->io_stdin.uid);
            // }
            // this->wait_child_processes();
            this->env.is_userpipe_out_error = true;
            // return;
        } else if(this->server.id2usershell[uid_to]->env.user_pipes[this->uid].isenable()) {
            string msg = "*** Error: the pipe #" + to_string(this->uid);
            msg += "->#" + to_string(uid_to) + " already exists. ***\n";
            this->server.safe_send(this->ufd, msg);
            // if (first_task->io_stdin.type == IOType::ToUserPipe) {
            //     this->wait_user_child_processes(first_task->io_stdin.uid);
            // }
            // this->wait_child_processes();
            this->env.is_userpipe_out_error = true;
            // return;
        } else {
            string msg = "*** " + this->name + " (#" + to_string(this->uid) + ") just piped '" + input;
            msg += "' to " + this->server.id2usershell[uid_to]->name + " (#" + to_string(uid_to) + ") ***\n";
            this->server.broadcast(msg);
            Pipe new_pipe;
            this->server.id2usershell[uid_to]->env.user_pipes[this->uid] = new_pipe;
            this->server.id2usershell[uid_to]->env.user_pipes[this->uid].createp();
        }
        cout << "user pipe" << endl;
    }

    for(unsigned i = 0; i < tasks.size(); i++) {
        while(tasks[i].exec(*this, i+1, tasks.size()) == NPStatus::ForkError) {
            usleep(1000);
        }
    }

    if (last_task->io_stdout.type == IOType::ToDelayedPipe) {
        this->env.move2_child_processes(last_task->io_stdout.pos, this->env.get_childprocess(0));
    } else if (last_task->io_stdout.type == IOType::ToUserPipe) {
        if(!this->env.is_userpipe_out_error)
            this->move_child_processes2user(last_task->io_stdout.uid,
                                        this->env.get_childprocess(0));
        else
            this->env.is_userpipe_out_error = false;
    }

    if (first_task->io_stdin.type == IOType::ToUserPipe) {
        if(!this->env.is_userpipe_in_error)
            this->wait_user_child_processes(first_task->io_stdin.uid);
        else
            this->env.is_userpipe_in_error = false;
    }
    this->wait_child_processes();
    this->env.clear_env();
}

Shell::~Shell() {
    for (int i = 0; i < 1500; i++) {
        for (auto& pid : this->env.get_childprocess(i)) {
            kill(pid, SIGKILL);
        }
    }
    for (auto& user : this->env.user_child_processes) {
        for (auto& pid : user.second) {
            kill(pid, SIGKILL);
        }
    }
    close(this->ufd);
}


