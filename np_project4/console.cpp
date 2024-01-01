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
    string socks_addr;
    string socks_port;
    Parser parser;
    string query_string = getenv("QUERY_STRING");
    auto socks_info = parser.parse(query_string, hosts);
    socks_addr = socks_info[0];
    socks_port = socks_info[1];
    parser.initial_page(hosts);
    try {
        boost::asio::io_context io_context;
        for(int i = 0; i < MAX_SERVER_NUM; i++) {
            if(hosts[i].isalive) {
                boost::asio::ip::tcp::resolver::query http_q(hosts[i].addr, hosts[i].port);
                boost::asio::ip::tcp::resolver::query socks_q(socks_addr, socks_port);
                make_shared<Client>(io_context, move(http_q), move(socks_q), hosts[i].id, hosts[i].file)->start();
            }
        }
        io_context.run();
    } catch(std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}