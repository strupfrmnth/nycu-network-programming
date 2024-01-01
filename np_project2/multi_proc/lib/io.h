#ifndef IO_H
#define IO_H

#include <string>

using namespace std;

enum IOType {
  Origin = 0,
  ToDelayedPipe,
  ToUserPipe,
  ToFile,
};

class IO
{
private:
    
public:
    int pos;
    int uid;
    IOType type;
    string file;
    IO();

};

#endif