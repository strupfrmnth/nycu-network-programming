#include <memory>
#include <boost/asio.hpp>
#include "request.h"
#include <set>

using namespace boost::asio::ip;
using namespace std;

class Connection;

class ConnectionMgr
{
private:
    set<shared_ptr<Connection>> connections_;
public:
    ConnectionMgr(/* args */);
    ~ConnectionMgr();
    void start(shared_ptr<Connection> c);
    void stop(shared_ptr<Connection> c);
    void stopall();
};

class Connection
 : public std::enable_shared_from_this<Connection>
{
private:
    tcp::socket socket_;
    ConnectionMgr& connectionmgr_;
    boost::asio::io_context& io_context_;
    enum { max_length = 1024 };
    char data_[max_length];
    Request request_;
    Parser parser_;
    void do_read();
    void parse_request(string data);
    void do_request();
    void set_env();
public:
    Connection(tcp::socket socket, ConnectionMgr& connectionmgr, boost::asio::io_context& io_context);
    void start();
    void stop(){socket_.close();};
    ~Connection();
};
