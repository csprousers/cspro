#pragma once

#include <zUtilO/zUtilO.h>
#include <zUtilO/ConnectionString.h>

class TemporaryFile;


// --------------------------------------------------------------------------
// ConnectionStringFileSimulator
//
// To facilitate opening non-file data sources from the MFC framework, this
// class creates temporary file paths for connection strings that would be
// mishandled by MFC (e.g., by adding working directory information to
// a connection string like: "|type=none"
// --------------------------------------------------------------------------

class CLASS_DECL_ZUTILO ConnectionStringFileSimulator
{
public:
    // For file-based connection strings, this returns a file path that can be processed by MFC.
    // For non-file-based connection strings, this returns the path of a temporary file that can
    // later be used to retrieve the connection string.
    std::wstring GetFilePath(const ConnectionString& connection_string);

    // Returns the connection string from a file path returned by GetFilePath
    ConnectionString GetConnectionString(const wchar_t* wide_file_path) const;

private:
    std::vector<std::tuple<std::shared_ptr<TemporaryFile>, ConnectionString>> m_nonFileBasedConnectionStrings;
};
