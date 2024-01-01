#include "firewall.h"
#include <fstream>
#include <iostream>

Firewall::Firewall() {
    ifstream firewall_stream("./socks.conf");
    string permit_c = "permit c ";
    string permit_b = "permit b ";
    string line;
    while(getline(firewall_stream, line)) {
        if(line.substr(0, permit_c.size()) == permit_c) {
            this->connect_rules.push_back(line.substr(permit_c.size()));
        } else if(line.substr(0, permit_b.size()) == permit_b) {
            this->bind_rules.push_back(line.substr(permit_b.size()));
        }
    }
}

bool Firewall::check(int status, string dstip) {
    if(status == Connect) {
        vector<string>::iterator it = this->connect_rules.begin();
        while(it != this->connect_rules.end()) {
            string accept_ip = (*it).substr(0, (*it).find('*'));
            if(dstip.substr(0, accept_ip.size()) == accept_ip) {
                return true;
            }
            it++;
        }
    } else if(status == Bind) {
        vector<string>::iterator it = this->bind_rules.begin();
        while(it != this->bind_rules.end()) {
            string accept_ip = (*it).substr(0, (*it).find('*'));
            if(dstip.substr(0, accept_ip.size()) == accept_ip) {
                return true;
            }
            it++;
        }
    }
    return false;
}

Firewall::~Firewall()
{
}