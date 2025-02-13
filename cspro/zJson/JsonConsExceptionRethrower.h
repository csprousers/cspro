#pragma once

#include <zJson/Json.h>


[[noreturn]] inline void RethrowJsonConsException(const jsoncons::json_exception& exception)
{
    throw JsonParseException(exception.CSPro_get_line_number(), std::string("JSON parsing error: ") + exception.what());
}
