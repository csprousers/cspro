#pragma once

#include <zUtilO/zUtilO.h>

class ConnectionStringFileSimulator;


// --------------------------------------------------------------------------
// ConnectionStringCommandLineParser
//
// Using the provided ConnectionStringFileSimulator, non-flag command line
// arguments are processed as connection strings.
// --------------------------------------------------------------------------

class CLASS_DECL_ZUTILO ConnectionStringCommandLineParser : public CCommandLineInfo
{
public:
    ConnectionStringCommandLineParser(cs::non_null_shared_or_raw_ptr<ConnectionStringFileSimulator> connection_string_file_simulator);

    const std::vector<std::wstring>& GetFilePaths() const { return m_filePaths; }

protected:
    void ParseParam(const TCHAR* pszParam, BOOL bFlag, BOOL bLast) override;

private:
    cs::non_null_shared_or_raw_ptr<ConnectionStringFileSimulator> m_connectionStringFileSimulator;
    std::vector<std::wstring> m_filePaths;
};
