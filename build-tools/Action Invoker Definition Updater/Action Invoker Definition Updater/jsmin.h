#pragma once

#define WIN_DESKTOP
#include <engine/StandardSystemIncludes.h>


namespace CSPro 
{
    int getc(FILE* stream);
    int putc(int ch, FILE* stream);
    std::string jsmin(const std::string& text);
}
