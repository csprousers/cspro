#pragma once


struct SymbolCompilerModifier
{
    std::stack<std::function<std::string()>> name_compiler;
    bool config_variable = false;
    bool declare_variable = false;
    bool persistent_variable = false;
};
