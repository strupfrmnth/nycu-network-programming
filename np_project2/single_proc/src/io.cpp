#include "io.h"

IO::IO()
{
    this->type = IOType::Origin;
    this->pos = 0;
    this->uid = -1;
    this->file = "";
}