#include "StdAfx.h"
#include "BinaryCommandLineInfo.h"


CSEntryBinaryCommandLineInfo::CSEntryBinaryCommandLineInfo()
    :   m_expectingPenFilePath(false)
{
}


void CSEntryBinaryCommandLineInfo::ParseParam(const TCHAR* const pszParam, const BOOL bFlag, const BOOL bLast)
{
    std::string param = TC::ToUtf8(pszParam);

    if( bFlag )
    {
        if( SO::EqualsOneOfNoCase(param, "pen", "binaryWin32", "binaryUnicode") )
        {
            if( !m_penFilePath.has_value() )
                m_penFilePath.emplace();
        }

        else if( SO::EqualsOneOfNoCase(param, "penName", "binaryName") )
        {
            m_expectingPenFilePath = true;
        }
    }

    else if( m_expectingPenFilePath ) // following /binaryName flag, pick up filename
    {
        m_penFilePath = std::move(param);
        m_expectingPenFilePath = false;
    }

    else
    {
        __super::ParseParam(pszParam, bFlag, bLast);
    }
}


void CSEntryBinaryCommandLineInfo::UpdateBinaryGen()
{
    if( m_penFilePath.has_value() )
    {
        // if no file path was specified, use the application's filename but replace the extension with .pen
        if( m_penFilePath->empty() )
            m_penFilePath = Path::ReplaceExtension(TC::ToUtf8(m_strFileName), FileExtensions::BinaryEntryPen);

        BinaryGen::SetCreatingPen(MakeFullPath(GetWorkingDirectory(), std::move(*m_penFilePath)));
    }
}
