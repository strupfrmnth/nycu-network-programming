#ifndef IO_H
#define IO_H

#include <string>

using namespace std;

enum IOType {
  Origin = 0,
  ToPipe,
  ToFile,
};

class IO
{
private:
    
public:
    int pos;
    IOType type;
    string file;
    IO();

};

#endif