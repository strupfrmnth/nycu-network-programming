#include <iostream>
#include <signal.h>
#include <unistd.h>
#include <cstring> //bzero
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "simple/lib/parser.h"
#include "simple/lib/task.h"
#include "simple/lib/environment.h"

using namespace std;

const int max_input_size = 15001;

static void signal_handler(int sig) {
    pid_t pid;
    int status;
    while((pid = waitpid(-1, &status, WNOHANG)) > 0);
}

int passive_tcp(int port) {
    const char *protocol = "TCP";
    // struct servent *pse;
    struct protoent *protocol_entry;
    struct sockaddr_in sin; /* an Internet endpoint address	*/
    int	s, type;
    bzero((char *)&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(port);
    if (strcmp(protocol, "UDP") == 0)
        type = SOCK_DGRAM;
    else
        type = SOCK_STREAM;
    if ( (protocol_entry = getprotobyname(protocol)) == 0) {
        cerr << "Fail to get protocol entry!!!" << endl;
        exit(1);
    }
    s = socket(PF_INET, type, protocol_entry->p_proto);
    if(s < 0) {
        cerr << "Fail to create socket!!!" << endl;
        exit(1);
    }
    /* Bind the socket */
    if(bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        cerr << "Fail to bind port!!!" << endl;
        exit(1);
    }
    if(type == SOCK_STREAM && listen(s, 0) < 0) {
        cerr << "Fail to listen port!!!" << endl;
        exit(1);
    }

    return s;
}

void create_shell(int ssock) {
    Environment env;
    env.set_env({"setenv", "PATH", "bin:."});
    dup2(ssock, STDOUT_FILENO);
    dup2(ssock, STDERR_FILENO);
    while(true) {
        // cout << "% ";
        write(ssock, "% ", strlen("% "));
        char inputs[max_input_size];
        memset(&inputs, 0, sizeof(inputs));
        if(recv(ssock, inputs, sizeof(inputs), 0) <= 0) {
            env.exit_shell({"exit"});
        }
        string input(inputs);
        std::size_t found = input.find("\n");
        if (found!=std::string::npos)
            input.pop_back();
        found = input.find("\r");
        if (found!=std::string::npos)
            input.pop_back();
        // if (!getline(cin, input)) {
        //     env.exit_shell({"exit"});
        // }
        Parser parser(input);
        vector<Task> tasks = parser.parse();
        if(tasks.empty()) {
            continue;
        }
        for(unsigned i = 0; i < tasks.size(); i++) {
            while(tasks[i].exec(env, i+1, tasks.size(), ssock) == NPStatus::ForkError) {
                usleep(1000);
            }
        }

        // solve for the excess of pipe capacity
        auto last_task = tasks[tasks.size()-1];
        // pid_t last_pid;
        if(last_task.io_stdout.type == IOType::ToPipe) {
            // last_pid = env.get_childprocess()[tasks.size()-1];
            env.add_childprocesses(last_task.io_stdout.pos,
                                    env.get_childprocess());
        }

        // wait for last child process
        // avoid print % before child process
        for(auto child_pid : env.get_childprocess()) {
            int status;
            waitpid(child_pid, &status, 0);
        }

        // special case: if child process has stdout and stderr but print stdout at other line
        // Here we wait child process printing stderr (Not in Spec)
        // if (last_task.io_stdout.type == IOType::ToPipe) {
        //     int status;
        //     int t = 0;
        //     while(t < 5) {
        //         if(waitpid(last_pid, &status, WNOHANG) == 0) {
        //             usleep(1000);
        //         }
        //         t++;
        //     }
        // }
        env.clear_env();
    }
    close(ssock);
}

int main(int argc, char *argv[]) {

    if(argc > 2) {
        cerr << "Argument set error" << endl;
        exit(1);
    }

    struct sockaddr_in client_addr;
    socklen_t addr_len;
    int msock, ssock;
    pid_t pid;

    int port = atoi(argv[1]);
    msock = passive_tcp(port);

    signal(SIGCHLD, signal_handler);

    while(true) {
        addr_len = sizeof(client_addr);
        ssock = accept(msock, (struct sockaddr *) &client_addr, &addr_len);
        cout << "accept" << endl;
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
            create_shell(ssock);
            close(ssock);
            exit(0);
        } else {
            close(ssock);
        }
    }

    return 0;
}