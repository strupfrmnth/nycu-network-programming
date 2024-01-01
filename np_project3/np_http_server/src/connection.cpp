#include "connection.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <iostream>
#include <cstring>

ConnectionMgr::ConnectionMgr(/* args */)
{
}

void ConnectionMgr::start(shared_ptr<Connection> c) {
    connections_.insert(c);
    c->start();
}

void ConnectionMgr::stop(shared_ptr<Connection> c) {
    connections_.erase(c);
    c->stop();
}

void ConnectionMgr::stopall() {
    for (auto& c : connections_) {
        c->stop();
    }
    connections_.clear();
}

ConnectionMgr::~ConnectionMgr()
{
}

Connection::Connection(tcp::socket socket, ConnectionMgr& connectionmgr, boost::asio::io_context& io_context)
 : socket_(std::move(socket)),
 connectionmgr_(connectionmgr),
 io_context_(io_context) {}

void Connection::do_read() {
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(data_, max_length),
        [this, self](boost::system::error_code ec, std::size_t length) {
            if(!ec) {
                string data(data_);
                // std::cout << data << endl;
                parse_request(data);
                do_request();
            } else if(ec != boost::asio::error::operation_aborted) {
                connectionmgr_.stop(shared_from_this());
            }
        }
    );
}

void Connection::parse_request(string data) {
    this->request_ = this->parser_.parse(data);
    // cout << this->request_.cgi << endl;
    this->request_.server_addr = this->socket_.local_endpoint().address().to_string();
    this->request_.server_port = to_string(this->socket_.local_endpoint().port());
    this->request_.remote_addr = this->socket_.remote_endpoint().address().to_string();
    this->request_.remote_port = to_string(this->socket_.remote_endpoint().port());
}

void Connection::do_request() {
    pid_t pid;
    // cout << "do request" << endl;
    io_context_.notify_fork(boost::asio::io_context::fork_prepare);
    if((pid = fork()) < 0) {
        io_context_.notify_fork(boost::asio::io_context::fork_parent);
    } else if(pid == 0) {
        io_context_.notify_fork(boost::asio::io_context::fork_child);
        this->set_env();
        dup2(this->socket_.native_handle(), 0);
        dup2(this->socket_.native_handle(), 1);
        dup2(this->socket_.native_handle(), 2);
        // close(this->socket_.native_handle());
        this->connectionmgr_.stopall();
        cout << "HTTP/1.1 200 OK\r\n";
        cout << "Content-Type: text/html\r\n";
        cout.flush();
        string path = "." + this->request_.cgi;
        char **argv = new char*[2];
        argv[0] = strdup(path.c_str());
        argv[1] = NULL;
        if(execv(path.c_str(), argv) < 0) {
            cerr << "Failed to execute cgi!!!" << endl;
        }
        boost::system::error_code ignored_ec;
        socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
        exit(0);
    } else {
        io_context_.notify_fork(boost::asio::io_context::fork_parent);
        int status = 0;
        waitpid(pid, &status, 0); 
        this->socket_.close();
    }
}

void Connection::set_env() {
    setenv("REQUEST_METHOD", this->request_.request_method.c_str(), 1);
    setenv("REQUEST_URI", this->request_.request_uri.c_str(), 1);
    setenv("QUERY_STRING", this->request_.query_string.c_str(), 1);
    setenv("SERVER_PROTOCOL", this->request_.server_protocol.c_str(), 1);
    setenv("HTTP_HOST", this->request_.http_host.c_str(), 1);
    setenv("SERVER_ADDR", this->request_.server_addr.c_str(), 1);
    setenv("SERVER_PORT", this->request_.server_port.c_str(), 1);
    setenv("REMOTE_ADDR", this->request_.remote_addr.c_str(), 1);
    setenv("REMOTE_PORT", this->request_.remote_port.c_str(), 1);
}

void Connection::start() {
    this->do_read();
}

Connection::~Connection()
{
}