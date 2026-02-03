#pragma once


namespace SQLiteSourceUpdater
{
    enum class SQLiteVersion { Public, SEE };

    enum class DllVersion { V1, V2, V3 };

    void Update(std::string& sqlite_code, bool is_header,
                SQLiteVersion sqlite_version, DllVersion dll_version = DllVersion::V3);
}



inline void SQLiteSourceUpdater::Update(std::string& sqlite_code, const bool is_header,
                                        const SQLiteVersion sqlite_version, const DllVersion dll_version/* = DllVersion::V3*/)
{
    ASSERT(sqlite_code.find('\r') == std::string::npos);

    std::string h_prefix = "#pragma once\n";
    std::string c_prefix;

    if( dll_version == DllVersion::V1 )
    {
        h_prefix.append("#include <SQLite/sqlite_dll.h>\n");

        c_prefix = "#ifndef ANDROID\n"
                   "#include <SQLite/sqlite_dll.h>\n"
                   "#endif\n";
    }

    else
    {
        if( dll_version == DllVersion::V3 )
            h_prefix.push_back('\n');

        h_prefix.append("#include <zSql/zSql.h>\n");

        if( dll_version == DllVersion::V3 )
            h_prefix.push_back('\n');

        c_prefix = "#include <zSql/zSql.h>\n";

        if( dll_version == DllVersion::V3 )
            c_prefix.push_back('\n');
    }

    if( sqlite_version == SQLiteVersion::SEE )
    {
        h_prefix.append("#define SQLITE_HAS_CODEC\n");

        if( dll_version == DllVersion::V3 )
            h_prefix.push_back('\n');
    }

    sqlite_code.insert(0, is_header ? h_prefix : c_prefix);
}
