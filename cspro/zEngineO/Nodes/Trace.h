#pragma once

#include <zLogicO/FunctionTable.h>


namespace Nodes
{
    struct Trace
    {
        enum class Action : int
        {
            TurnOff = -5,
            FileOnClear = -4,
            FileOn = -3, 
            WindowOn = -2,
            LogicText = -1,
            UserText = 0
        };

        FunctionCode function_code;
        Action action;
        int argument;
    };
}
