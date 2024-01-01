#ifndef HOST_H
#define HOST_H

#include <string>

using namespace std;

class Host
{
private:
public:
    string addr;
    string port;
    string file;
    string id;
    bool isalive;
    Host();
    ~Host();
};

class Parser
{
private:
public:
    Parser(/* args */);
    void parse(string query_string, Host (&hosts)[5]);
    void initial_page(Host (&hosts)[5]);
    ~Parser();
};

#endif