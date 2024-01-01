#include <boost/asio.hpp>
#include "np_console/lib/client.h"
#include "np_console/lib/host.h"
#include <iostream>
#include <string>
#include <fstream>

using namespace std;

const int MAX_SERVER_NUM = 5;

int main(int argc, char *argv[]) {
    Host hosts[MAX_SERVER_NUM];
    Parser parser;
    string query_string = getenv("QUERY_STRING");
    parser.parse(query_string, hosts);
    parser.initial_page(hosts);
    // for(int i = 0; i < MAX_SERVER_NUM; i++) {
    //     if(hosts[i].isalive) {
    //         cout << hosts[i].addr << endl;
    //         cout << hosts[i].file << endl;
    //     }
    // }
    // string file_path = "./test_case/" + hosts[0].file;
    // string line;
    // ifstream ifs(file_path);
    // cout << file_path << "\n";
    // cout.flush();
    // vector<string> commands;
    // // ifs.open(file_path);
    // while(getline(ifs, line)) {
    //     commands.push_back(line);
    // }
    // // ifs.close();
    // cout << commands.size();
    // cout << commands[0] << endl;
    // cout << commands[1] << endl;
    // cout.flush();
    try {
        boost::asio::io_context io_context;
        for(int i = 0; i < MAX_SERVER_NUM; i++) {
            if(hosts[i].isalive) {
                boost::asio::ip::tcp::resolver::query q(hosts[i].addr, hosts[i].port);
                make_shared<Client>(io_context, move(q), hosts[i].id, hosts[i].file)->start();
            }
        }
        io_context.run();
    } catch(std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}