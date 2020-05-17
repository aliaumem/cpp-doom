extern "C" {
#include "i_system_ext.h"
}

#include <stdexcept>

extern "C" void ExitGracefully(int code)
{
    throw std::runtime_error{std::to_string(code)};
}