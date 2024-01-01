#ifndef SERVER_H
#define SERVER_H

#include <boost/asio.hpp>
#include "connection.h"

class Server
{
private:
    boost::asio::io_context& io_context;
    boost::asio::signal_set signal_;
    boost::asio::ip::tcp::acceptor acceptor_;
    ConnectionMgr connectionmgr_;
    void do_accept();
    void do_signal();

public:
    Server(boost::asio::io_context& io_context, short port);
    void run();
    ~Server();
};

#endif