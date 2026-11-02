#pragma once

#include <zLogicO/zLogicO.h>


class ZLOGICO_API LogicFormatter
{
public:
    static CString FormatLogic(int lexer, const std::wstring& logic);
};
