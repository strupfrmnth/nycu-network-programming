#include "pipe.h"
#include <unistd.h>
#include <stdexcept>

Pipe::Pipe() {
    this->pfd[0] = this->pfd[1] = -1;
}

void Pipe::createp() {
    if (pipe(this->pfd) < 0) {
        throw std::runtime_error("fail to create pipe");
    }
}

void Pipe::dup2in(int stdfd) {
    dup2(this->pfd[this->in], stdfd);
}

void Pipe::dup2out(int stdfd) {
    dup2(this->pfd[this->out], stdfd);
}

void Pipe::closep() {
    if(this->pfd[this->in] >= 0) {
        close(this->pfd[this->in]);
        this->pfd[this->in] = -1;
    }
    if(this->pfd[this->out] >= 0) {
        close(this->pfd[this->out]);
        this->pfd[this->out] = -1;
    }
}

bool Pipe::isenable() {
    if(this->pfd[this->in] == -1 && this->pfd[this->out] == -1)
        return false;
    else
        return true;
}

Pipe& Pipe::operator=(const Pipe& other) {
    if (this == &other)
        return *this;
    pfd[0] = other.pfd[0];
    pfd[1] = other.pfd[1];
    return *this;
}

Pipe::~Pipe()
{
}