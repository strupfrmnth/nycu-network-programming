#include "io.h"

IO::IO()
{
    this->type = IOType::Origin;
    this->pos = 0;
    this->file = "";
}