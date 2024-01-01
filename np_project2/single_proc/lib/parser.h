#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include "task.h"

using namespace std;

class Parser
{
private:
    string input;
    vector<string> commands;
public:
    Parser(string command);
    vector<Task> parse();
    string& ltrim(string& s);
    ~Parser();
};

#endif