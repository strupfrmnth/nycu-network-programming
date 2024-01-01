#include "server_and_shell.h"
#include "parser.h"
#include "task.h"
#include "pipe.h"
#include <unistd.h>
#include <iostream>
#include <memory.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/shm.h>

Server_info* Server::users = NULL;
int Server::users_uid = 0;

Server_info* Shell::users = NULL;
int Shell::users_uid = 0;

Server::Server(int msock, key_t shmkey) {
    this->msock = msock;
    this->shmkey = shmkey;
    int shmid = shmget(shmkey, sizeof(struct Server_info)*30, 0666 | IPC_CREAT);
	if(shmid < 0) {
        cerr << "server: can't create shared memory\n";
        cout << "here\n";
        exit(1);
    }
		
	if((users = (struct Server_info*)shmat(shmid, NULL, 0)) == (void *)-1) 
		cerr << "shmat error\n";
    for(int i = 0; i < 30; i++) {
        users[i].id = -1;
        users[i].fd = -1;
        users[i].msock = msock;
        strcpy(users[i].addr, "");
        strcpy(users[i].port, "");
        strcpy(users[i].username, "");
        strcpy(users[i].msg, "");
        for(int j = 0; j < 30; j++) {
            users[i].fifo[j].fd = -1;
            strcpy(users[i].fifo[j].name, "");
            for(int k = 0; k < 1000; k++) {
                users[i].userpipe_pids[j][k] = -1;
            }
        }
        // cout << "done\n";
    }
    cout << "done\n";
}

shared_ptr<Shell> Server::create_shell(sockaddr_in* client_info, int ufd) {
    shared_ptr<Shell> shell_ptr(new Shell(client_info, ufd, this->shmkey));
    if (shell_ptr->uid == -1) {
        cerr << "Failed to create user: uid or ufd is occupied!!!" << endl;
        exit(1);
    }
    return shell_ptr;
}

// weak_ptr<Shell> Server::getuserbyid(int uid) {
//     return this->id2usershell[uid];
// }

// static void server_pass(int sig) {
//     ;
//     signal(sig, server_pass);
// }

void Server::run() {
    pid_t pid;
    while(true) {
        struct sockaddr_in client_info;
        socklen_t addrlen = sizeof(client_info);
        int ssock = accept(this->msock, (struct sockaddr *) &client_info, &addrlen);
        if(ssock < 0) {
            if(errno == EINTR)
                continue;
            cerr << "Server fail to accept, errno: " << errno << endl;
            exit(1);
        }
        if((pid = fork()) < 0) {
            cerr << "Fail to fork!!!" << endl;
            exit(1);
        } else if(pid == 0) {
            close(msock);
            auto shell = this->create_shell(&client_info, ssock);
            shell->run();
            if(shell.use_count() > 3) {
                cerr << "Failed to delete user: shell's shared_ptr is owned by others\n";
            }
            shell->clear_processes();
            shell.reset();
            close(ssock);
            exit(0);
        } else {
            cout << "User connected" << endl;
            close(ssock);
        }
    }
    
}

Server::~Server()
{
}

template <class T>
ssize_t Shell::safe_send(int fd, const T& v) {
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

Shell::Shell(sockaddr_in* client_info, int ufd, key_t shmkey) {
    this->ufd = ufd;
    int shmid = shmget(shmkey, sizeof(struct Server_info)*30, 0666 | IPC_CREAT);
	if(shmid < 0)
		cerr << "server: can't create shared memory\n";
	if((users = (struct Server_info*)shmat(shmid, NULL, 0)) == (void *)-1) 
		cerr << "shmat error\n";
    this->uid = -1;
    for(int i = 0; i < 30; i++) {
        if(users[i].id == -1) {
            users[i].id = i+1;
            users[i].fd = ufd;
            this->uid = i+1;
            users_uid = i+1;
            break;
        }
    }
    this->isalive = true;
    string str(inet_ntoa(client_info->sin_addr));
    this->addr = str;
    this->port = to_string(ntohs(client_info->sin_port));
    this->name = "(no name)";
    strcpy(users[this->uid-1].addr, this->addr.c_str());
    strcpy(users[this->uid-1].port, this->port.c_str());
    strcpy(users[this->uid-1].username, this->name.c_str());
    this->pid = getpid();
    users[this->uid-1].pid = this->pid;
    this->builtin_setenv({"setenv", "PATH", "bin:."});
}

bool Shell::user_exists(int uid) {
    if(uid <= 30) {
        if(users[uid-1].id != -1) {
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

void Shell::deleteuser(int uid) {
    users[this->uid-1].id = -1;
    users[this->uid-1].fd = -1;
    strcpy(users[this->uid-1].addr, "");
    strcpy(users[this->uid-1].port, "");
    strcpy(users[this->uid-1].username, "");
    strcpy(users[this->uid-1].msg, "");
    users[this->uid-1].pid = -1;
    for(int i = 0; i < 30; i++) {
        if(users[this->uid-1].fifo[i].fd != -1) {
            close(users[this->uid-1].fifo[i].fd);
            users[this->uid-1].fifo[i].fd = -1;
        }
        if(strcmp(users[this->uid-1].fifo[i].name, "")) {
            unlink(users[this->uid-1].fifo[i].name);
            strcpy(users[this->uid-1].fifo[i].name, "");
        }
        for(int j = 0; j < 1000; j++) {
            users[this->uid-1].userpipe_pids[i][j] = -1;
        }
    }
}

void Shell::builtin_exit() {
    this->deleteuser(this->uid);
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
    for (int i = 0; i < 30; i++) {
        if (users[i].id != -1) {
            cout << i+1 << "\t" << users[i].username << "\t";
            cout << users[i].addr << ":" << users[i].port;
            if(this->uid == i+1) {
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
        if(this->user_exists(uid_to)) {
            string message = "*** " + this->name + " told you ***: " + msg + "\n";
            strcpy(users[uid_to-1].msg, message.c_str());
            kill(users[uid_to-1].pid, SIGUSR1);
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
        this->broadcast(yell_msg);
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
        for(int i = 0; i < 30; i++) {
            if(strcmp(users[i].username, new_name.c_str()) == 0) {
                issame = true;
                break;
            }
        }
        if(issame) {
            cout << "*** User '" + new_name + "' already exists. ***\n";
        } else {
            string msg = "*** User from " + this->addr + ":" + this->port;
            msg += " is named '" + new_name + "'. ***\n";
            this->broadcast(msg);
            strcpy(users[this->uid-1].username, new_name.c_str());
            this->name = new_name;
        }
    }

    dup2(inherit_stdout, STDOUT_FILENO);
    dup2(inherit_stderr, STDERR_FILENO);
    close(inherit_stdout);
    close(inherit_stderr);
}

ssize_t Shell::broadcast(const string& msg) {
    ssize_t rtn = 0;
    for (int i = 0; i < 30; i++) {
        if(users[i].id != -1) {
            strcpy(users[i].msg, msg.c_str());
            kill(users[i].pid, SIGUSR1);
        }
    }
    return rtn;
}

ssize_t Shell::send_welcomemsg(int fd) {
    static string msg(
        "****************************************\n"
        "** Welcome to the information server. **\n"
        "****************************************\n"
    );
    return this->safe_send(fd, msg);
}

ssize_t Shell::send_percentmsg(int fd) {
    static string msg("% ");
    return this->safe_send(fd, msg);
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
    for (auto& pid : users[this->uid-1].userpipe_pids[uid-1]) {
        if(pid != -1)
            waitpid(pid, &status, 0);
    }
    for(int i = 0; i < 1000; i++) {
        users[this->uid-1].userpipe_pids[uid-1][i] = -1;
    }
    return status;
}

// bool Shell::move_child_processes2user(int uid, vector<pid_t>& pids) {
//     if(this->server->user_exists(uid)) {
//         this->server->id2usershell[uid]->env.move2_user_child_processes(this->uid, pids);
//         return true;
//     }
//     return false;
// }

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
    bool need_userpipe_broadcast = false;
    string userpipe_broadcast_msg("");
    if(first_task->io_stdin.type == IOType::ToUserPipe) {
        int uid_from = first_task->io_stdin.uid;
        if(!this->user_exists(uid_from)) {
            string msg = "*** Error: user #" + to_string(uid_from) + " does not exist yet. ***\n";
            this->safe_send(this->ufd, msg);
            Pipe fake_pipe;
            fake_pipe.pfd[fake_pipe.in] = open("/dev/null", O_RDWR);
            this->env.last_pipe = fake_pipe;
            this->env.is_userpipe_in_error = true;
            // this->wait_child_processes();
            // return;
        } else if(users[this->uid-1].fifo[uid_from-1].fd != -1) {
            // if(this->env.getuserpipe(uid_from)->isenable()) {
                need_userpipe_broadcast = true;
                string name_str(users[uid_from-1].username);
                string msg = "*** " + this->name + " (#" + to_string(this->uid);
                msg += ") just received from " + name_str;
                msg += " (#" + to_string(uid_from) + ") by '" + input + "' ***\n";
                // this->broadcast(msg);
                userpipe_broadcast_msg += msg;
                Pipe fake_pipe;
                fake_pipe.pfd[fake_pipe.in] = users[this->uid-1].fifo[uid_from-1].fd;
                this->env.last_pipe = fake_pipe;
                // Pipe new_pipe;
                // this->env.user_pipes[uid_from] = new_pipe;
            // }
            // else {
            //     string msg = "*** Error: the pipe #" + to_string(uid_from) + "->#" + to_string(this->uid) + " does not exist yet. ***\n";
            //     this->safe_send(this->ufd, msg);
            //     this->wait_child_processes();
            //     return;
            // }
        } else {
            string msg = "*** Error: the pipe #" + to_string(uid_from) + "->#" + to_string(this->uid) + " does not exist yet. ***\n";
            this->safe_send(this->ufd, msg);
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
        if(!this->user_exists(uid_to)) {
            string msg = "*** Error: user #" + to_string(uid_to) + " does not exist yet. ***\n";
            this->safe_send(this->ufd, msg);
            // if (first_task->io_stdin.type == IOType::ToUserPipe) {
            //     this->wait_user_child_processes(first_task->io_stdin.uid);
            // }
            // this->wait_child_processes();
            this->env.is_userpipe_out_error = true;
            // return;
        } else if(users[uid_to-1].fifo[this->uid-1].fd != -1) {
            string msg = "*** Error: the pipe #" + to_string(this->uid);
            msg += "->#" + to_string(uid_to) + " already exists. ***\n";
            this->safe_send(this->ufd, msg);
            // if (first_task->io_stdin.type == IOType::ToUserPipe) {
            //     this->wait_user_child_processes(first_task->io_stdin.uid);
            // }
            // this->wait_child_processes();
            this->env.is_userpipe_out_error = true;
            // return;
        } else {
            need_userpipe_broadcast = true;
            string name_str(users[uid_to-1].username);
            string msg = "*** " + this->name + " (#" + to_string(this->uid) + ") just piped '" + input;
            msg += "' to " + name_str + " (#" + to_string(uid_to) + ") ***\n";
            // this->broadcast(msg);
            userpipe_broadcast_msg += msg;
            string fifo_path = "./user_pipe/" + to_string(this->uid) + "->" + to_string(uid_to);
            if(mkfifo(fifo_path.c_str(), 0600) < 0) {
                cerr << "Failed to create FIFO!!!\n";
            } else {
                strcpy(users[uid_to-1].fifo[this->uid-1].name, fifo_path.c_str());
                kill(users[uid_to-1].pid, SIGUSR2);
                this->env.userpipefd = open(fifo_path.c_str(), O_WRONLY);
                // this->env.fifo_name = fifo_path;
                // this->env.userpipe_read_pid = users[uid_to-1].pid;
            }
            // Pipe new_pipe;
            // this->server->id2usershell[uid_to]->env.user_pipes[this->uid] = new_pipe;
            // this->server->id2usershell[uid_to]->env.user_pipes[this->uid].createp();
        }
    }

    if(need_userpipe_broadcast)
        this->broadcast(userpipe_broadcast_msg);

    for(unsigned i = 0; i < tasks.size(); i++) {
        while(tasks[i].exec(*this, i+1, tasks.size()) == NPStatus::ForkError) {
            usleep(1000);
        }
    }
    if(this->env.userpipefd != -1) {
        close(this->env.userpipefd);
        this->env.userpipefd = -1;
    }
    if(first_task->io_stdin.type == IOType::ToUserPipe && last_task->io_stdout.type == IOType::ToDelayedPipe) {
        if(!this->env.is_userpipe_in_error) {
            vector<pid_t> up_pids;
            for (auto& pid : users[this->uid-1].userpipe_pids[first_task->io_stdin.uid-1]) {
                if(pid != -1)
                    up_pids.push_back(pid);
            }
            for(int i = 0; i < 1000; i++) {
                users[this->uid-1].userpipe_pids[first_task->io_stdin.uid-1][i] = -1;
            }
            int real_pos = (last_task->io_stdout.pos + this->env.current_pos) % 1500;
            this->env.unlink_pos[first_task->io_stdin.uid-1] = real_pos;
            this->env.move2_child_processes(last_task->io_stdout.pos, up_pids);
        } else {
            this->env.is_userpipe_in_error = false;
        }
        this->env.move2_child_processes(last_task->io_stdout.pos, this->env.get_childprocess(0));
    } else if(last_task->io_stdout.type == IOType::ToDelayedPipe) {
        this->env.move2_child_processes(last_task->io_stdout.pos, this->env.get_childprocess(0));
    } else if(first_task->io_stdin.type == IOType::ToUserPipe && last_task->io_stdout.type == IOType::ToUserPipe) {
        // this->move_child_processes2user(last_task->io_stdout.uid,
        //                             this->env.get_childprocess(0));
        if(!this->env.is_userpipe_out_error) {
            for(int i = 0; i < int(this->env.get_childprocess(0).size()); i++) {
                if(i < 1000)
                    users[last_task->io_stdout.uid-1].userpipe_pids[this->uid-1][i] = this->env.get_childprocess(0)[i];
            }
        } else {
            this->env.is_userpipe_out_error = false;
        }
        if(!this->env.is_userpipe_in_error) {
            this->wait_user_child_processes(first_task->io_stdin.uid);
            this->env.unlink_pos[first_task->io_stdin.uid-1] = this->env.current_pos;
        } else {
            this->env.is_userpipe_in_error = false;
        }
    } else if(last_task->io_stdout.type == IOType::ToUserPipe) {
        if(!this->env.is_userpipe_out_error) {
            for(int i = 0; i < int(this->env.get_childprocess(0).size()); i++) {
                if(i < 1000)
                    users[last_task->io_stdout.uid-1].userpipe_pids[this->uid-1][i] = this->env.get_childprocess(0)[i];
            }
        } else {
            this->env.is_userpipe_out_error = false;
        }
    } else if(first_task->io_stdin.type == IOType::ToUserPipe) {
        if(!this->env.is_userpipe_in_error) {
            this->wait_user_child_processes(first_task->io_stdin.uid);
            this->env.unlink_pos[first_task->io_stdin.uid-1] = this->env.current_pos;
        } else {
            this->env.is_userpipe_in_error = false;
        }
    }

    if(last_task->io_stdout.type != IOType::ToUserPipe) {
        this->wait_child_processes();
    }
    for(int i = 0; i < 30; i++) {
        if(this->env.unlink_pos[i] == this->env.current_pos) {
            //close fifo
            users[this->uid-1].fifo[i].fd = -1;
            unlink(users[this->uid-1].fifo[i].name);
            strcpy(users[this->uid-1].fifo[i].name, "");
            this->env.unlink_pos[i] = -1;
        }
    }
    this->env.clear_env();
}

void Shell::clear_processes() {
    string msg = "*** User '" + this->name + "' left. ***\n";
    this->broadcast(msg);
    users[this->uid-1].id = -1;
    users[this->uid-1].fd = -1;
    strcpy(users[this->uid-1].addr, "");
    strcpy(users[this->uid-1].port, "");
    strcpy(users[this->uid-1].username, "");
}

void Shell::run() {
    int max_buffer_size = 15001;
    // struct sigaction act1, act2;
    // act1.sa_handler = this->send2itself;
    // act2.sa_handler = this->open_fifo_read;
    // act1.sa_flags = SA_RESTART;
    // act2.sa_flags = SA_RESTART;
    // sigemptyset(&act1.sa_mask);
    // sigemptyset(&act2.sa_mask);
    // sigaddset(&act1.sa_mask, SIGUSR1);
    // sigaddset(&act2.sa_mask, SIGUSR2);
    // sigaction(SIGUSR1, &act1, 0);
    // sigaction(SIGUSR2, &act2, 0);
    signal (SIGUSR1, this->send2itself);
	signal (SIGUSR2, this->open_fifo_read);
    signal (SIGINT, this->sig_handler);
	signal (SIGQUIT, this->sig_handler);
	signal (SIGTERM, this->sig_handler);
    this->send_welcomemsg(this->ufd);
    this->broadcast("*** User '(no name)' entered from " + this->addr + ":" + this->port + ". ***\n");
    this->send_percentmsg(this->ufd);
    while(true) {
        char buffer[max_buffer_size];
        memset(&buffer, '\0', sizeof(buffer));
        if(recv(this->ufd, buffer, sizeof(buffer), 0) <= 0) {
            this->execute("exit");
            break;
        } else {
            this->execute(string(buffer));
            if(this->isalive) {
                this->send_percentmsg(this->ufd);
            } else {
                break;
            }
        }
    }
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
    users[this->uid-1].id = -1;
    users[this->uid-1].fd = -1;
    strcpy(users[this->uid-1].addr, "");
    strcpy(users[this->uid-1].port, "");
    strcpy(users[this->uid-1].username, "");
    strcpy(users[this->uid-1].msg, "");
    users[this->uid-1].pid = -1;
    for(int i = 0; i < 30; i++) {
        if(users[this->uid-1].fifo[i].fd != -1) {
            close(users[this->uid-1].fifo[i].fd);
            users[this->uid-1].fifo[i].fd = -1;
        }
        if(strcmp(users[this->uid-1].fifo[i].name, "")) {
            unlink(users[this->uid-1].fifo[i].name);
            strcpy(users[this->uid-1].fifo[i].name, "");
        }
        for(int j = 0; j < 1000; j++) {
            users[this->uid-1].userpipe_pids[i][j] = -1;
        }
    }
    close(this->ufd);
}


