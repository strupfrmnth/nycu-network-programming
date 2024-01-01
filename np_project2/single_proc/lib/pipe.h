#ifndef PIPE_H
#define PIPE_H

class Pipe
{
private:

public:
    int pfd[2];
    const int in = 0;
    const int out = 1;
    Pipe();
    void createp();
    void dup2in(int stdfd);
    void dup2out(int stdfd);
    void closep();
    bool isenable();
    Pipe& operator=(const Pipe& other);
    ~Pipe();
};

#endif