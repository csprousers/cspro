#include "stdafx.h"
#include "JsonWriter.h"
#include <zToolsO/Special.h>


JsonWriter& JsonWriter::Write(const std::wstring& value)
{
    return Write(UTF8_TODO::GetUtf8(value));
}


JsonWriter& JsonWriter::Write(const CString& value)
{
    return Write(UTF8_TODO::GetUtf8(value));
}


JsonWriter& JsonWriter::WriteEngineValue(const double value)
{
    if( IsSpecial(value) )
    {
        return Write(SpecialValues::ValueToString(value));
    }

    else
    {
        return Write(value);
    }
}


JsonWriter& JsonWriter::WriteEngineValue(const std::wstring& value)
{
    return Write(value);
}


JsonWriter& JsonWriter::WriteEngineValue(const std::variant<double, std::wstring>& value)
{
    return std::holds_alternative<double>(value) ? WriteEngineValue(std::get<double>(value)) :
                                                   Write(UTF8_TODO::GetUtf8(std::get<std::wstring>(value)));
}


JsonWriter& JsonWriter::WriteDate(const std::string_view key_sv, const int64_t date)
{
    return Key(key_sv).Write(DateTime::TimeToRFC3339(date));
}


JsonWriter& JsonWriter::WritePath(std::string path)
{
    return Write(PortableFunctions::PathToForwardSlash(std::move(path)));
}


JsonWriter& JsonWriter::WriteRelativePath(const std::string& path)
{
    return WritePath(GetRelativePath(path));
}


JsonWriter& JsonWriter::WriteRelativePathWithDirectorySupport(const std::string& path)
{
    // the relative path calculation functions don't work properly with a directory if it does not end in a slash
    if( !path.empty() && path.back() != Path::NativeSlashChar && PortableFunctions::FileIsDirectory(path) )
    {
        std::string modified_path = GetRelativePath(PortableFunctions::PathEnsureTrailingSlash(path));

        if( modified_path.empty() )
            return Write(".");

        ASSERT(modified_path.back() == Path::NativeSlashChar);
        modified_path.pop_back();
        return WritePath(std::move(modified_path));
    }

    else
    {
        return WriteRelativePath(path);
    }
}
