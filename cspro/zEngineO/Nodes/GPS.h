#pragma once

#include <zLogicO/FunctionTable.h>


namespace Nodes
{
    struct GPS
    {
        enum class Command : int
        {
            Open = 1,
            Close,
            Status,
            Read,
            ReadLast,
            Latitude,
            Longitude,
            Altitude,
            Satellites,
            Accuracy,
            ReadTime,
            Distance,
            ReadInteractive,
            Select
        };

        FunctionCode function_code;
        Command command;
        int options[1];
    };
}
