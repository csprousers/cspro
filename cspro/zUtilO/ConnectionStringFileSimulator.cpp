#include "StdAfx.h"
#include "ConnectionStringFileSimulator.h"
#include "TemporaryFile.h"


std::wstring ConnectionStringFileSimulator::GetFilePath(const ConnectionString& connection_string)
{
    // although there may be non-path characters such as | in the connection string,
    // the MFC framework handles these fine
    if( connection_string.HasFilePath() )
    {
        // however, only do this if the path, along with any properties, is of a proper size
        std::wstring wide_file_path = TC::ToWide(connection_string.ToString());

        if( wide_file_path.length() < _MAX_PATH )
            return wide_file_path;
    }

    // for non-files, reuse a temporary file if one already exists for this connection string
    for( const auto& [temporary_file, non_file_based_connection_string] : m_nonFileBasedConnectionStrings )
    {
        if( connection_string.Equals(non_file_based_connection_string) )
            return TC::ToWide(temporary_file->GetPath());
    }

    // create a temporary file
    const auto& addition = m_nonFileBasedConnectionStrings.emplace_back(std::make_unique<TemporaryFile>(), connection_string);
    ASSERT(PortableFunctions::FileIsRegular(std::get<0>(addition)->GetPath()));

    return TC::ToWide(std::get<0>(addition)->GetPath());
}


ConnectionString ConnectionStringFileSimulator::GetConnectionString(const wchar_t* const wide_file_path) const
{
    const std::string file_path = TC::ToUtf8(wide_file_path);

    for( const auto& [temporary_file, non_file_based_connection_string] : m_nonFileBasedConnectionStrings )
    {
        if( file_path == temporary_file->GetPath() )
            return non_file_based_connection_string;
    }

    return ConnectionString(file_path);
}
