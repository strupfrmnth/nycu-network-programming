#include <string>

using namespace std;

class Request
{
private:
public:
    enum { Unknown = 0, Connect = 1, Bind = 2 };
    uint8_t vn;
    uint8_t cd;
    uint16_t dstport;
    uint32_t dstip;
    uint64_t userid;
    string hostname;
    Request();
    Request& operator=(const Request& other);
    ~Request();
};

class Parser
{
private:

public:
    Parser();
    Request parse(uint8_t* buffer, ssize_t buffer_size);
    bool issocks4a(uint32_t dstip);
    ~Parser();
};
