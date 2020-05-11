extern "C" {
#include "i_system_ext.h"
}

extern "C" void ExitGracefully(int code)
{
    throw code;
}