#include "StdAfx.h"
#include "BinaryCommandLineInfo.h"


CSEntryBinaryCommandLineInfo::CSEntryBinaryCommandLineInfo()
    :   m_bGeneratePen(false),
        m_bExpectingPenFilename(false)
{
}


void CSEntryBinaryCommandLineInfo::ParseParam(const TCHAR* pszParam, BOOL bFlag, BOOL bLast)
{
    if( bFlag )
    {
        if( _tcsicmp(pszParam, _T("pen")) == 0 || _tcsicmp(pszParam, _T("binaryWin32")) == 0 || _tcsicmp(pszParam, _T("binaryUnicode")) == 0 )
        {
            m_bGeneratePen = true;
        }

        else if( _tcsicmp(pszParam, _T("penName")) == 0 || _tcsicmp(pszParam, _T("binaryName")) == 0 )
        {
            m_bExpectingPenFilename = true;
        }
    }

    else if( m_bExpectingPenFilename ) // following /binaryName flag, pick up filename
    {
        m_penFilename = pszParam;
        m_bExpectingPenFilename = false;
    }

    else
    {
        CIMSACommandLineInfo::ParseParam(pszParam, bFlag, bLast);
    }
}


const std::wstring& CSEntryBinaryCommandLineInfo::GetPenFilename()
{
    // if no name was specified, use the .ent filename but replace the extension with .pen
    if( m_penFilename.empty() )
        m_penFilename = PortableFunctions::PathRemoveFileExtension<CString>(m_strFileName) + FileExtensions::WithDot::BinaryEntryPen;

    return m_penFilename;
}


void CSEntryBinaryCommandLineInfo::UpdateBinaryGen()
{
    BinaryGen::m_bGeneratingBinary = m_bGeneratePen;

    if( m_bGeneratePen )
        BinaryGen::m_sBinaryName = GetPenFilename();
}
