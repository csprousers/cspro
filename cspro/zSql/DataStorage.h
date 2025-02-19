#pragma once


namespace Sqlite
{
    template<typename T>
    constexpr const char* GetDataType();

    template<> constexpr const char* GetDataType<int>()         { return "INTEGER"; }
    template<> constexpr const char* GetDataType<long>()        { return "INTEGER"; }
    template<> constexpr const char* GetDataType<size_t>()      { return "INTEGER"; }
    template<> constexpr const char* GetDataType<double>()      { return "REAL"; }
    template<> constexpr const char* GetDataType<std::string>() { return "TEXT"; }
}
