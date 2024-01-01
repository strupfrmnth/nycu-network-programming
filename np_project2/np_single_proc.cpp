#include <iostream>
#include <signal.h>
#include <cstring> //bzero
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "single_proc/lib/server_and_shell.h"

using namespace std;

static void signal_handler(int sig) {
    pid_t pid;
    int status;
    while((pid = waitpid(-1, &status, WNOHANG)) > 0);
}

int main(int argc, char *argv[]) {

    if(argc > 2) {
        cerr << "Argument set error" << endl;
        exit(1);
    }
    int port = atoi(argv[1]);

    signal(SIGCHLD, signal_handler);

    int sockfd = 0;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd == -1) {
        cerr << "Fail to create socket!!!" << endl;
        exit(1);
    }

    struct sockaddr_in server_info;

    bzero(&server_info, sizeof(server_info));
    server_info.sin_family = AF_INET;
    server_info.sin_addr.s_addr = INADDR_ANY;
    server_info.sin_port = htons(port);

    int flag = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int)) < 0) {
        cerr << "setsockopt(SO_REUSEADDR) failed!!!" << endl;
        exit(1);
    }

    bind(sockfd, (struct sockaddr*)&server_info, sizeof(server_info));
    listen(sockfd, 5);
    cout << "Server is listening on port " << port << endl;

    Server server(sockfd);
    server.run();
    
    return 0;
}