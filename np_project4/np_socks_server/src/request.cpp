#include "request.h"
#include <vector>
#include <limits>
#include <iostream>

Request::Request(): vn(0), cd(Unknown), dstport(0), dstip(0), userid(0), hostname() {}

Request& Request::operator=(const Request& other) {
    if(this == &other)
        return *this;
    this->vn = other.vn;
    this->cd = other.cd;
    this->dstport = other.dstport;
    this->dstip = other.dstip;
    this->userid = other.userid;
    this->hostname = other.hostname;
    return *this;
}

Request::~Request()
{
}

Parser::Parser()
{
}

Request Parser::parse(uint8_t* buffer, ssize_t buffer_size) {
    Request request;

    if(buffer[0] == 0x4) {
        request.vn = buffer[0];
    }

    if(buffer[1] == 0x1 || buffer[1] == 0x2) {
        request.cd = buffer[1];
    }

    for(ssize_t i = 2; i <= 3; i++) {
        request.dstport = request.dstport << 8 | buffer[i];
    }

    for(ssize_t i = 4; i <= 7; i++) {
        request.dstip = request.dstip << 8 | buffer[i];
    }

    ssize_t i;
    for (i = 8; i < buffer_size && buffer[i] != 0; i++) {
        request.userid = request.userid << 8 | buffer[i];
    }

    if(this->issocks4a(request.dstip) && i+1 < buffer_size) {
        request.hostname.assign((char*)&buffer[i+1], buffer_size-i-1);
        char last_words = request.hostname[request.hostname.size()-1];
        if(last_words == '\0') {
            request.hostname.pop_back();
        }
    }
    
    return request;
}

bool Parser::issocks4a(uint32_t dstip) {
    if(dstip > 0 && dstip <= numeric_limits<uint8_t>::max()) {
        return true;
    }
    else {
        return false;
    }
}

Parser::~Parser()
{
}