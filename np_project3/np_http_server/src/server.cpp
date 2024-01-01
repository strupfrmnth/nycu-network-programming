#include "server.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <iostream>

Server::Server(boost::asio::io_context& io_context, short port)
    : io_context(io_context), 
    signal_(io_context, SIGCHLD), 
    acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {}

void Server::do_signal() {
    this->signal_.async_wait(
        [this](const boost::system::error_code& ec, int signum) {
            if (acceptor_.is_open()) {
                int status = 0;
                while (waitpid(-1, &status, WNOHANG) > 0);
                do_signal();
            }
        }
    );
}

void Server::do_accept() {
    this->acceptor_.async_accept(
        [this](boost::system::error_code ec, tcp::socket socket) {
            if(!ec) {
                // std::cout << "accept" << std::endl;
                // std::make_shared<Connection>(std::move(socket))->start();
                this->connectionmgr_.start(std::make_shared<Connection>(std::move(socket), this->connectionmgr_, io_context));
            }
            do_accept();
        }
    );
}

void Server::run() {
    this->do_signal();
    this->do_accept();
    this->io_context.run();
}

Server::~Server()
{
}