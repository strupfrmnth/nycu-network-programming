#ifndef SERVER_H
#define SERVER_H

#include <netinet/in.h>
#include <map>
#include <queue>
#include <vector>
#include <memory>
#include <functional>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <iostream>
#include "environment.h"

using namespace std;

class Shell;

struct FIFO {
    int	fd;
	char name[64+1];
};

struct Server_info {
    int msock;
    int id;
    int fd;
    pid_t pid;
    char addr[30];
    char port[30];
    char username[30];
    char msg[1025];
    FIFO fifo[30];
    pid_t userpipe_pids[30][1000];
};

class Server
{
private:
    static Server_info *users;
    static int users_uid;
public:
    int msock;
    key_t shmkey;
    queue<shared_ptr<Shell>> garbage_shells;
    Server(int msock, key_t shmkey);
    void run();
    shared_ptr<Shell> create_shell(sockaddr_in* client_info, int ufd);
    // vector<weak_ptr<Shell>> getusers();
    // weak_ptr<Shell> getuserbyid(int uid);
    ~Server();
};

class Shell
{
private:
    static Server_info *users;
    static int users_uid;
public:
    int ufd;
    int uid;
    pid_t pid;
    bool isalive;
    string addr;
    string port;
    string name;
    static Server_info *server_info;
    Environment env;
    Shell(sockaddr_in* client_info, int ufd, key_t shmkey);
    bool user_exists(int uid);
    void deleteuser(int uid);
    void builtin_exit();
    void builtin_printenv(vector<string> args);
    void builtin_setenv(vector<string> args);
    void builtin_who();
    void builtin_tell(vector<string> args);
    void builtin_yell(vector<string> args);
    void builtin_name(vector<string> args);
    void execute(string input);
    ssize_t broadcast(const string& msg);
    ssize_t send_welcomemsg(int fd);
    ssize_t send_percentmsg(int fd);
    template <class T> ssize_t safe_send(int fd, const T& v);
    int wait_child_processes();
    int wait_user_child_processes(int uid);
    bool move_child_processes2user(int uid, vector<pid_t>& pids);
    void clear_leftuser_processes();
    void clear_processes();
    void run();
    static void send2itself(int sig) {
        if(users_uid > 0) {
            int ufd = users[users_uid-1].fd;
            string msg(users[users_uid-1].msg);
            write(ufd, msg.c_str(), msg.length());
            strcpy(users[users_uid-1].msg, "");
        }
        signal(sig, send2itself);
    };
    static void open_fifo_read(int sig) {
        if(users_uid > 0) {
            for(int i = 0; i < 30; i++) {
                if(users[users_uid-1].fifo[i].fd == -1 && strcmp(users[users_uid-1].fifo[i].name, "")) {
                    users[users_uid-1].fifo[i].fd = open(users[users_uid-1].fifo[i].name, O_RDONLY | O_NONBLOCK);
                    cout << users[users_uid-1].fifo[i].fd << endl;
                }
            }
        }
        signal(sig, open_fifo_read);
    }
    static void rm_user(void)
    {
        /* close the fds */
        close (STDIN_FILENO);
        close (STDOUT_FILENO);
        close (STDERR_FILENO);
        /* close fifo fds */
        for (int i = 0; i < 30; ++i) {
            if (users[users_uid-1].fifo[i].fd && users[users_uid-1].fifo[i].fd!=-1) {
                close (users[users_uid-1].fifo[i].fd);
                unlink (users[users_uid-1].fifo[i].name);
            }
        }
        /* clear the user entry */
        memset (&users[users_uid-1], 0, sizeof (Server_info));
    }
    static void sig_handler(int sig) {
        rm_user();
        shmdt(users);
        signal(sig, sig_handler);
    }
    ~Shell();
};

#endif