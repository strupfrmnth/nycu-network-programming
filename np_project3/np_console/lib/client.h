#ifndef CLIENT_H
#define CLIENT_H

#include <memory>
#include <string>
#include <vector>
#include <boost/asio.hpp>

using namespace std;

class Client
 : public enable_shared_from_this<Client>
{
private:
    boost::asio::ip::tcp::socket socket_;
    boost::asio::ip::tcp::resolver resolver_;
    boost::asio::ip::tcp::resolver::query query_;
    enum { max_length = 1024 };
    char data_[max_length];
    string id_;
    vector<string> commands_;
    int display_commands_counter_;
    int write_commands_counter_;
    void do_resolve();
    void do_connect(boost::asio::ip::tcp::resolver::iterator it);
    void do_read();
    void do_write();
    void display_output(string output, bool iscommand);
    void to_html_format(string& output);
public:
    Client(boost::asio::io_context& io_context,
            boost::asio::ip::tcp::resolver::query q,
            string id, string file);
    void start();
    ~Client();
};




#endif