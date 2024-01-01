#include "request.h"
#include <vector>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

Request::Request() {
    this->request_method = "";
    this->request_uri = "";
    this->query_string = "";
    this->server_protocol = "";
    this->http_host = "";
    this->server_addr = "";
    this->server_port = "";
    this->remote_addr = "";
    this->remote_port = "";
    this->cgi = "";
}

Request& Request::operator=(const Request& other) {
    if(this == &other)
        return *this;
    this->request_method = other.request_method;
    this->request_uri = other.request_uri;
    this->query_string = other.query_string;
    this->server_protocol = other.server_protocol;
    this->http_host = other.http_host;
    this->server_addr = other.server_addr;
    this->server_port = other.server_port;
    this->remote_addr = other.remote_addr;
    this->remote_port = other.remote_port;
    this->cgi = other.cgi;
    return *this;
}

Request::~Request()
{
}

Parser::Parser()
{
}

Request Parser::parse(string data) {
    Request request;
    vector<string> request_vector;
    boost::split(request_vector, data, boost::is_any_of(" \r\n"), boost::token_compress_on);
    vector<string>::iterator it = request_vector.begin();
    int count = 1;
    while(it!=request_vector.end() || count<=5) {
        if(count == 1)
            request.request_method = *it;
        else if(count == 2)
            request.request_uri = *it;
        else if(count == 3)
            request.server_protocol = *it;
        else if(count == 5)
            request.http_host = *it;
        count++;
        it++;
    }
    if(request.request_uri.find("?") != string::npos) {
        vector<string> query_vector;
        boost::split(query_vector, request.request_uri, boost::is_any_of("?"), boost::token_compress_on);
        request.cgi = query_vector[0];
        request.query_string = query_vector[1];
    } else {
        request.cgi = request.request_uri;
    }
    return request;
}

Parser::~Parser()
{
}