#include "client.h"
#include <fstream>
#include <iostream>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

Client::Client(boost::asio::io_context& io_context,
            boost::asio::ip::tcp::resolver::query q,
            string id, string file)
    : socket_(io_context),
    resolver_(io_context),
    query_(move(q)),
    id_(id)
{
    string file_path = "test_case/" + file;
    string line;
    ifstream ifs(file_path);
    while(getline(ifs, line)) {
        this->commands_.push_back(line);
    }
    this->display_commands_counter_ = 0;
    this->write_commands_counter_ = 0;
}

void Client::to_html_format(string& output) {
    boost::algorithm::replace_all(output, "&", "&amp;");
    boost::algorithm::replace_all(output, "\"", "&quot;");
    boost::algorithm::replace_all(output, "\'", "&apos;");
    boost::algorithm::replace_all(output, "<", "&lt;");
    boost::algorithm::replace_all(output, ">", "&gt;");
    boost::algorithm::replace_all(output, "\r\n", "\n");
    boost::algorithm::replace_all(output, "\n", "<br>");
}

void Client::display_output(string output, bool iscommand) {
    this->to_html_format(output);
    if(iscommand) {
        boost::format script("<script>document.getElementById('%1%').innerHTML += '<b>%2%</b>';</script>");
        cout << script % this->id_ % output;
    } else {
        boost::format script("<script>document.getElementById('%1%').innerHTML += '%2%';</script>");
        cout << script % this->id_ % output;
    }
    cout.flush();
}

void Client::do_write() {
    string command = this->commands_[this->write_commands_counter_++];
    command += "\n";
    this->display_output(command, true);
    auto self(shared_from_this());
    boost::asio::async_write(socket_,
        boost::asio::buffer(command, command.size()),
        [this, self](boost::system::error_code ec, size_t length) {
            if (!ec) {
                // string cmd = this->commands_[this->display_commands_counter_++];
                // cmd += "\n";
                // this->display_output(cmd, true);
                this->do_read();
            }
        }
    );
}

void Client::do_read() {
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(this->data_, max_length), 
        [this, self](boost::system::error_code ec, size_t length) {
            if(!ec) {
                string data(this->data_, this->data_+length);
                if(data.find("% ") != string::npos) {
                    this->display_output(data, false);
                    // string cmd = this->commands_[this->display_commands_counter_++];
                    // cmd += "\n";
                    // this->display_output(cmd, true);
                    // socket_.write_some(boost::asio::buffer(cmd));
                    // this->do_read();
                    this->do_write();
                } else {
                    this->display_output(data, false);
                    this->do_read();
                }
            }
        }
    );
}

void Client::do_connect(boost::asio::ip::tcp::resolver::iterator it) {
    auto self(shared_from_this());
    socket_.async_connect(*it, 
        [this, self](boost::system::error_code ec) {
            if(!ec) {
                this->do_read();
            }
        }
    );
}

void Client::do_resolve() {
    auto self(shared_from_this());
    resolver_.async_resolve(this->query_, 
        [this, self](boost::system::error_code ec, boost::asio::ip::tcp::resolver::iterator it) {
            if(!ec) {
                this->do_connect(it);
            }
        }
    );
}

void Client::start() {
    this->do_resolve();
}

Client::~Client()
{
}