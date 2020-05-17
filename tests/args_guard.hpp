#pragma once

#ifdef CRISPY_DOOM
extern "C" {
#include <m_argv.h>
}
#elif CPP_DOOM
#include <m_argv.hpp>
#endif

class args_guard
{
public:
    args_guard()
     : args_guard(std::array<char const*, 0>{})
    {}

    template <std::size_t N>
    args_guard(std::array<char const*, N> params)
    {
        static std::array<char const*, N+1> argv;
        argv[0] = "/";
        std::copy(std::begin(params), std::end(params), std::begin(argv) + 1);

        myargc = N + 1;
        myargv = const_cast<char**>(&argv.at(0));
    }

    ~args_guard()
    {
        myargc = 0;
        myargv = nullptr;
    }
};
