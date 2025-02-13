#pragma once


struct SymbolCompilerModifier
{
    std::stack<std::function<std::wstring()>> name_compiler;
    bool config_variable = false;
    bool persistent_variable = false;
};
