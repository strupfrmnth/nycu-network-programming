#ifndef SERVER_H
#define SERVER_H

#include <netinet/in.h>
#include <map>
#include <queue>
#include <vector>
#include <memory>
#include <functional>
#include "environment.h"

using namespace std;

class Shell;

class Server
{
private:
public:
    int msock;
    int maxfd;
    fd_set	afds;		/* active file descriptor set */
    vector< shared_ptr<Shell> > id2usershell;
    map<int, shared_ptr<Shell>> fd2usershell;
    map<int, string> id2username;
    priority_queue<int, vector<int>, greater<int> > available_ids;
    queue<shared_ptr<Shell>> garbage_shells;
    Server(int msock);
    void run();
    shared_ptr<Shell> create_shell(sockaddr_in* client_info, int ufd);
    int create_id();
    vector<weak_ptr<Shell>> getusers();
    void deleteuser(Shell& shell);
    weak_ptr<Shell> getuserbyid(int uid);
    void clear_garbage_shells();
    ssize_t broadcast(const string& msg);
    ssize_t send_welcomemsg(int fd);
    ssize_t send_percentmsg(int fd);
    template <class T> ssize_t safe_send(int fd, const T& v);
    bool user_exists(int uid);
    ~Server();
};

class Shell
{
private:
public:
    int ufd;
    int uid;
    bool isalive;
    string addr;
    string port;
    string name;
    Server& server;
    Environment env;
    Shell(sockaddr_in* client_info, int ufd, int uid, Server& server);
    void builtin_exit();
    void builtin_printenv(vector<string> args);
    void builtin_setenv(vector<string> args);
    void builtin_who();
    void builtin_tell(vector<string> args);
    void builtin_yell(vector<string> args);
    void builtin_name(vector<string> args);
    void execute(string input);
    int wait_child_processes();
    int wait_user_child_processes(int uid);
    bool move_child_processes2user(int uid, vector<pid_t>& pids);
    ~Shell();
};

#endif