#include "server.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <iostream>

using namespace std;

Server::Server(boost::asio::io_context& io_context, short port)
    : io_context(io_context), 
    signal_(io_context, SIGCHLD), 
    acceptor_(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
    dst_socket_(io_context),
    src_socket_(io_context),
    resolver_(io_context) {}

void Server::do_signal() {
    signal_.async_wait(
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
    acceptor_.async_accept(src_socket_,
        [this](boost::system::error_code ec) {
            if(!ec) {
                do_socks4();
            }
            do_accept();
        }
    );
}

void Server::do_reply(uint8_t code) {
    array<uint8_t, 8> reply;
    auto port = dst_endpoint_.port();
    auto addr = boost::asio::ip::make_address_v4("0.0.0.0").to_bytes();
    auto ptr = reply.data();

    ptr[0] = 0;
    ptr[1] = code;
    ptr[2] = (port >> 8) & 0xff;
    ptr[3] = port & 0xff;
    memcpy(&ptr[4], (uint8_t*)&addr, 4);
    send(src_socket_.native_handle(), &reply, reply.size(), 0);
}

void Server::do_bind_reply(uint8_t code, boost::asio::ip::tcp::endpoint ep) {
    array<uint8_t, 8> reply;
    auto port = ep.port();
    auto addr = boost::asio::ip::make_address_v4("0.0.0.0").to_bytes();
    auto ptr = reply.data();

    ptr[0] = 0;
    ptr[1] = code;
    ptr[2] = (port >> 8) & 0xff;
    ptr[3] = port & 0xff;
    memcpy(&ptr[4], (uint8_t*)&addr, 4);
    send(src_socket_.native_handle(), &reply, reply.size(), 0);
}

void Server::read_from_dst() {
    dst_socket_.async_read_some(boost::asio::buffer(dst_buffer_),
        [this](boost::system::error_code ec, size_t length) {
            if(!ec) {
                write_to_src(length);
            }
        }
    );
}

void Server::write_to_src(size_t length) {
    async_write(src_socket_, boost::asio::buffer(dst_buffer_, length),
        [this](boost::system::error_code ec, size_t length) {
            if(!ec) {
                read_from_dst();
            }
        }
    );
}

void Server::read_from_src() {
    src_socket_.async_read_some(boost::asio::buffer(src_buffer_),
        [this](boost::system::error_code ec, size_t length) {
            if(!ec) {
                write_to_dst(length);
            }
        }
    );
}

void Server::write_to_dst(size_t length) {
    async_write(dst_socket_, boost::asio::buffer(src_buffer_, length),
        [this](boost::system::error_code ec, size_t length) {
            if(!ec) {
                read_from_src();
            }
        }
    );
}

void Server::do_connect() {
    dst_socket_.async_connect(dst_endpoint_, 
        [this](boost::system::error_code ec) {
            if(!ec) {
                do_reply(0x5A);
                read_from_dst();
                read_from_src();
            }
        }
    );
}

void Server::do_resolve(boost::asio::ip::tcp::resolver::query q) {
    resolver_.async_resolve(q, 
        [this](boost::system::error_code ec, boost::asio::ip::tcp::resolver::iterator it) {
            if(!ec) {
                dst_endpoint_ = it->endpoint();
                do_connect();
            } else {
                cerr << ec.message() << endl;
                cerr << "cannot resolve" << endl;
            }
        }
    );
}

void Server::do_socks4() {
    io_context.notify_fork(boost::asio::io_context::fork_prepare);
    pid_t pid;
    while((pid = fork()) < 0) {
        int status = 0;
        waitpid(-1, &status, 0);
    }
    if(pid == 0) {
        io_context.notify_fork(boost::asio::io_context::fork_child);
        signal_.cancel();
        acceptor_.close();
        src_socket_.async_read_some(
            boost::asio::buffer(buffer_, buffer_.size()),
            [this](boost::system::error_code ec, size_t length) {
                request = parser.parse(buffer_.data(), length);
                string dstip = boost::asio::ip::address_v4((uint_least32_t)request.dstip).to_string();
                string cmd = request.cd == Request::Connect ? "Connect" : "Bind";
                if(firewall.check(request.cd, dstip)) {
                    cout<<"<S_IP>: " << src_socket_.remote_endpoint().address().to_string() << endl;
                    cout<<"<S_PORT>: " << src_socket_.remote_endpoint().port() << endl;
                    cout<<"<D_IP>: " << dstip << endl;
                    cout<<"<D_PORT>: " << request.dstport << endl;
                    cout<<"<Command>: " << cmd << endl;
                    cout<<"<Reply>: Accept" << endl;
                    if(cmd == "Connect") {
                        string req_port = to_string(request.dstport);
                        if(parser.issocks4a(request.dstip)) {
                            boost::asio::ip::tcp::resolver::query q(request.hostname, req_port);
                            do_resolve(q);
                        } else {
                            string req_dstip = to_string(request.dstip);
                            boost::asio::ip::tcp::resolver::query q(req_dstip, req_port);
                            do_resolve(q);
                        }
                        // do_resolve();
                    } else {
                        boost::asio::ip::tcp::endpoint init_ep(boost::asio::ip::tcp::v4(), 0);
                        boost::asio::ip::tcp::acceptor bind_acceptor(io_context, init_ep);
                        auto ep = bind_acceptor.local_endpoint();
                        do_bind_reply(0x5A, ep);
                        // cout << "after bind first reply" << endl;
                        // bind_acceptor.async_accept(dst_socket_,
                        //     [this, &bind_acceptor, &ep](boost::system::error_code ec) {
                        //         if(bind_acceptor.is_open() && !ec) {
                        //             cout << "bind second reply" << endl;
                        //             do_bind_reply(0x5A, ep);
                        //             cout << "after bind second reply" << endl;
                        //             read_from_dst();
                        //             read_from_src();
                        //         } else if(!bind_acceptor.is_open()) {
                        //             cout << "acceptor not open" << endl;
                        //         } else {
                        //             cerr << ec.message() << endl;
                        //         }
                        //     }
                        // );
                        bind_acceptor.accept(dst_socket_);
                        do_bind_reply(0x5A, ep);
                        read_from_dst();
                        read_from_src();
                    }
                } else {
                    cout<<"<S_IP>: " << src_socket_.remote_endpoint().address().to_string() << endl;
                    cout<<"<S_PORT>: " << src_socket_.remote_endpoint().port() << endl;
                    cout<<"<D_IP>: " << dstip << endl;
                    cout<<"<D_PORT>: " << request.dstport << endl;
                    cout<<"<Command>: " << cmd << endl;
                    cout<<"<Reply>: Reject" << endl;
                    do_reply(0x5B);
                }
            }
        );
    } else {
        io_context.notify_fork(boost::asio::io_context::fork_parent);
        src_socket_.close();
        dst_socket_.close();
    }
}

void Server::run() {
    this->do_signal();
    this->do_accept();
    this->io_context.run();
}

Server::~Server()
{
}