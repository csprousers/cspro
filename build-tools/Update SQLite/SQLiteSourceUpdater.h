#pragma once


namespace SQLiteSourceUpdater
{
    enum class Version { Public, SEE };

    void Update(std::string& sqlite_code, bool is_header, Version version);
}


inline void SQLiteSourceUpdater::Update(std::string& sqlite_code, const bool is_header, const Version version)
{
    ASSERT(sqlite_code.find('\r') == std::string::npos);

    std::string prefix;

    if( is_header )
    {
        prefix = "#pragma once\n\n"
                 "#include <zSql/zSql.h>\n\n";

        if( version == Version::SEE )
            prefix.append("#define SQLITE_HAS_CODEC\n\n");
    }

    else
    {
        prefix = "#include <zSql/zSql.h>\n\n";
    }

    sqlite_code.insert(0, prefix);
}
