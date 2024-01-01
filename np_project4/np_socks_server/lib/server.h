#ifndef SERVER_H
#define SERVER_H

#include <boost/asio.hpp>
#include <boost/array.hpp>
#include "request.h"
#include "firewall.h"

class Server
{
private:
    boost::asio::io_context& io_context;
    boost::asio::signal_set signal_;
    boost::asio::ip::tcp::acceptor acceptor_;
    boost::asio::ip::tcp::socket dst_socket_;
    boost::asio::ip::tcp::socket src_socket_;
    boost::asio::ip::tcp::resolver resolver_;
    boost::asio::ip::tcp::endpoint dst_endpoint_;
    boost::array<uint8_t, 4096> buffer_;
    array<uint8_t, 4096> dst_buffer_;
    array<uint8_t, 4096> src_buffer_;
    Parser parser;
    Request request;
    Firewall firewall;
    void do_accept();
    void do_signal();
    void do_socks4();
    void do_resolve(boost::asio::ip::tcp::resolver::query q);
    void do_connect();
    void do_reply(uint8_t code);
    void do_bind_reply(uint8_t code, boost::asio::ip::tcp::endpoint ep);
    void read_from_dst();
    void write_to_src(size_t length);
    void read_from_src();
    void write_to_dst(size_t length);

public:
    Server(boost::asio::io_context& io_context, short port);
    void run();
    ~Server();
};

#endif