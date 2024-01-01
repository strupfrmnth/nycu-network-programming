#include <boost/asio.hpp>
#include "np_socks_server/lib/server.h"
#include <iostream>

int main(int argc, char *argv[]) {
    if(argc < 2) {
        std::cerr << "Need to choose a port!" << std::endl;
        return 1;
    }
    try {
        boost::asio::io_context io_context;
        Server server(io_context, std::atoi(argv[1]));
        server.run();
    } catch(std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}