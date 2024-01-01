#ifndef CLIENT_H
#define CLIENT_H

#include <memory>
#include <string>
#include <vector>
#include <boost/asio.hpp>
#include <boost/array.hpp>

using namespace std;

class Client
 : public enable_shared_from_this<Client>
{
private:
    boost::asio::ip::tcp::socket socket_;
    boost::asio::ip::tcp::resolver resolver_;
    boost::asio::ip::tcp::resolver::query http_query_;
    boost::asio::ip::tcp::resolver::query socks_query_;
    enum { max_length = 1024 };
    char data_[max_length];
    boost::array<uint8_t, 4096> reply_buffer_;
    string id_;
    vector<string> commands_;
    int display_commands_counter_;
    int write_commands_counter_;
    void do_resolve();
    void do_connect(boost::asio::ip::tcp::resolver::iterator it);
    void do_http_resolve();
    void read_reply();
    void do_read();
    void do_write();
    void display_output(string output, bool iscommand);
    void to_html_format(string& output);
public:
    Client(boost::asio::io_context& io_context,
            boost::asio::ip::tcp::resolver::query http_q,
            boost::asio::ip::tcp::resolver::query socks_q,
            string id, string file);
    void start();
    ~Client();
};




#endif