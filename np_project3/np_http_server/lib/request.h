#include <string>

using namespace std;

class Request
{
private:
public:
    string request_method;
    string request_uri;
    string query_string;
    string server_protocol;
    string http_host;
    string server_addr;
    string server_port;
    string remote_addr;
    string remote_port;
    string cgi;
    Request();
    Request& operator=(const Request& other);
    ~Request();
};

class Parser
{
private:
    /* data */
public:
    Parser();
    Request parse(string data);
    ~Parser();
};
