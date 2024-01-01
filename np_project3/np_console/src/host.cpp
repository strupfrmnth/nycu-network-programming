#include "host.h"
#include <vector>
#include <iostream>
#include <boost/format.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

Host::Host() {
    this->addr = "";
    this->port = "";
    this->file = "";
    this->id = "s";
    this->isalive = false;
}

Host::~Host()
{
}

Parser::Parser() {
}

void Parser::parse(string query_string, Host (&hosts)[5]) {
    vector<string> hosts_info;
    boost::split(hosts_info, query_string, boost::is_any_of("&"), boost::token_compress_on);
    vector<string>::iterator it = hosts_info.begin();
    int count = 0;
    int host_id = 0;
    while(it != hosts_info.end() && !(*it).empty()) { // last one is empty
        vector<string> key_and_value;
        boost::split(key_and_value, *it, boost::is_any_of("="), boost::token_compress_on);
        if(!key_and_value[1].empty()) {
            hosts[host_id].isalive = true;
            hosts[host_id].id += to_string(host_id);
            if(count % 3 == 0) {
                hosts[host_id].addr = key_and_value[1];
            } else if(count % 3 == 1) {
                hosts[host_id].port = key_and_value[1];
            } else {
                hosts[host_id].file = key_and_value[1];
                host_id++;
            }
        }
        it++;
        count++;
    }
}

void Parser::initial_page(Host (&hosts)[5]) {
    cout << "Content-type: text/html\r\n\r\n";
    string head = 
    "<!DOCTYPE html>\n"
    "<html lang=\"en\">\n"
        "<head>\n"
            "<meta charset=\"UTF-8\" />\n"
            "<title>NP Project 3 Sample Console</title>\n"
            "<link\n"
                "rel=\"stylesheet\"\n"
                "href=\"https://cdn.jsdelivr.net/npm/bootstrap@4.5.3/dist/css/bootstrap.min.css\"\n"
                "integrity=\"sha384-TX8t27EcRE3e/ihU7zmQxVncDAy5uIKz4rEkgIXeMed4M0jlfIDPvg6uqKI2xXr2\"\n"
                "crossorigin=\"anonymous\"\n"
            "/>\n"
            "<link\n"
                "href=\"https://fonts.googleapis.com/css?family=Source+Code+Pro\"\n"
                "rel=\"stylesheet\"\n"
            "/>\n"
            "<link\n"
                "rel=\"icon\"\n"
                "type=\"image/png\"\n"
                "href=\"https://cdn0.iconfinder.com/data/icons/small-n-flat/24/678068-terminal-512.png\"\n"
            "/>\n"
            "<style>\n"
                "* {\n"
                    "font-family: 'Source Code Pro', monospace;\n"
                    "font-size: 1rem !important;\n"
                "}\n"
                "body {\n"
                    "background-color: #212529;\n"
                "}\n"
                "pre {\n"
                    "color: #cccccc;\n"
                "}\n"
                "b {\n"
                    "color: #01b468;\n"
                "}\n"
            "</style>\n"
        "</head>\n";
    boost::format body(
        "<body>\n"
            "<table class=\"table table-dark table-bordered\">\n"
            "<thead>\n"
                "<tr>\n"
                    "%1%"
                // <th scope="col">nplinux1.cs.nctu.edu.tw:1234</th>
                // <th scope="col">nplinux2.cs.nctu.edu.tw:5678</th>
                "</tr>\n"
            "</thead>\n"
            "<tbody>\n"
                "<tr>\n"
                    "%2%"
                // <td><pre id="s0" class="mb-0"></pre></td>
                // <td><pre id="s1" class="mb-0"></pre></td>
                "</tr>\n"
            "</tbody>\n"
            "</table>\n"
        "</body>\n"
    );
    string tail = "</html>\n";
    string ths = "";
    string tds = "";
    for(int i = 0; i < 5; i++) {
        if(hosts[i].isalive) {
            ths += "<th scope=\"col\">" + hosts[i].addr + ":" + hosts[i].port + "</th>\n";
            tds += "<td><pre id=\"" + hosts[i].id + "\" class=\"mb-0\"></pre></td>\n";
        }
    }
    cout << head << (body%ths%tds) << tail;
    cout.flush();
}

Parser::~Parser()
{
}