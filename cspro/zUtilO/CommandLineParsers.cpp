#include "StdAfx.h"
#include "CommandLineParsers.h"
#include "ConnectionStringFileSimulator.h"


// --------------------------------------------------------------------------
// ConnectionStringCommandLineParser
// --------------------------------------------------------------------------

ConnectionStringCommandLineParser::ConnectionStringCommandLineParser(cs::non_null_shared_or_raw_ptr<ConnectionStringFileSimulator> connection_string_file_simulator)
    :   m_connectionStringFileSimulator(std::move(connection_string_file_simulator))
{
}


void ConnectionStringCommandLineParser::ParseParam(const TCHAR* pszParam, const BOOL bFlag, const BOOL bLast)
{
    if( !bFlag )
    {
        ConnectionString connection_string(TC::ToUtf8(pszParam));
        connection_string.AdjustRelativePath(GetWorkingDirectory());
        pszParam = m_filePaths.emplace_back(m_connectionStringFileSimulator->GetFilePath(connection_string)).c_str();
    }

    __super::ParseParam(pszParam, bFlag, bLast);
}
