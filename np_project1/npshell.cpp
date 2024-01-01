#include <iostream>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include "lib/parser.h"
#include "lib/task.h"
#include "lib/environment.h"

using namespace std;

static void signal_handler(int sig) {
    pid_t pid;
    int status;
    while((pid = waitpid(-1, &status, WNOHANG)) > 0);
}

int main() {
    signal(SIGCHLD, signal_handler);
    Environment env;
    env.set_env({"setenv", "PATH", "bin:."});
    while(true) {
        cout << "% ";
        string input;
        if (!getline(cin, input)) {
            env.exit_shell({"exit"});
        }
        Parser parser(input);
        vector<Task> tasks = parser.parse();
        if(tasks.empty()) {
            continue;
        }
        for(unsigned i = 0; i < tasks.size(); i++) {
            while(tasks[i].exec(env, i+1, tasks.size()) == NPStatus::ForkError) {
                usleep(1000);
            }
        }

        // solve for the excess of pipe capacity
        auto last_task = tasks[tasks.size()-1];
        // pid_t last_pid;
        if (last_task.io_stdout.type == IOType::ToPipe) {
            // last_pid = env.get_childprocess()[tasks.size()-1];
            env.add_childprocesses(last_task.io_stdout.pos,
                                    env.get_childprocess());
        }

        // wait for last child process
        // avoid print % before child process
        for (auto child_pid : env.get_childprocess()) {
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
    return 0;
}