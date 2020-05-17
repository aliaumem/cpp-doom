#pragma once

#ifdef CRISPY_DOOM
extern "C" {
#include <r_defs.h>
}
#elif CPP_DOOM
#include <r_defs.hpp>
#endif

extern lighttable_t*** scalelight;
extern lighttable_t**  scalelightfixed;
extern lighttable_t*** zlight;

class lights_guard
{
public:
    ~lights_guard()
    {
        scalelight = nullptr;
        scalelightfixed = nullptr;
        zlight = nullptr;
    }
};