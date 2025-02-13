//***************************************************************************
//  File name: SPECFILE.CPP
//
//  Description:
//       Implementation for CSpecFile class.
//
//  NOTE: Place change history in the *.h file.
//
//***************************************************************************

#include "StdAfx.h"
#include "Specfile.h"
#include <zToolsO/Utf8Convert.h>


/////////////////////////////////////////////////////////////////////////////
//
//                               CSpecFile::CSpecFile
//
/////////////////////////////////////////////////////////////////////////////
CSpecFile::CSpecFile(BOOL bIsSilent /*=FALSE*/)  {

    m_bIsSilent = bIsSilent;
    m_bIsReading = FALSE;
    m_bPushBack = FALSE;

#ifdef WIN_DESKTOP
    m_pFileException = new CFileException();
#endif
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CSpecFile::~CSpecFile
//
/////////////////////////////////////////////////////////////////////////////
CSpecFile::~CSpecFile(void)  {
#ifdef WIN_DESKTOP
    ASSERT_VALID(m_pFileException);
    m_pFileException->Delete();
#endif
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CSpecFile::Open
//
/////////////////////////////////////////////////////////////////////////////
#ifdef WIN_DESKTOP
/*virtual*/ BOOL CSpecFile::Open (const csprochar* pszFileName, UINT uOpenFlag) {


    ASSERT (pszFileName != NULL);

    // 20100818 this assert was failing when being called in a tab application (due to additional flags)
    //ASSERT (uOpenFlag == CFile::modeRead || uOpenFlag == CFile::modeWrite);

    bool bDone = false;
    m_csFileName = pszFileName;
    m_uFileState = SF_OK;
    if (uOpenFlag == CFile::modeRead)
    {
        m_bIsReading = TRUE;
        while (!bDone) {
            //if (!CStdioFile::Open(pszFileName, uOpenFlag | CFile::shareDenyWrite | CFile::typeBinary, m_pFileException)) {
            if (!CStdioFileUnicode::Open(pszFileName, uOpenFlag | CFile::shareDenyWrite | CFile::typeText, m_pFileException)) { // 20111215 for unicode, also changing type to text
                bDone = true;
                CString csLoad;
                csLoad.LoadString(IDS_ERROROPENREAD);
                m_csErrorMessage.Format (csLoad, pszFileName);
                if (m_pFileException->m_cause == CFileException::accessDenied) {
                    csLoad.LoadString(IDS_ACCESSDENIED);
                    m_csErrorMessage += csLoad;
                    if (!IsSilent())  {
                        if (AfxMessageBox(m_csErrorMessage, MB_RETRYCANCEL | MB_ICONEXCLAMATION) == IDRETRY) {
                            bDone = false;
                            continue;
                        }
                        TRACE(m_csErrorMessage);
                        m_uFileState = SF_ABORT;
                        break;
                    }
                }
                else if (m_pFileException->m_cause == CFileException::sharingViolation) {
                    csLoad.LoadString(IDS_INUSEBYANOTHER);
                    m_csErrorMessage += csLoad;
                    if (!IsSilent())  {
                        if (AfxMessageBox(m_csErrorMessage, MB_RETRYCANCEL | MB_ICONEXCLAMATION) == IDRETRY) {
                            bDone = false;
                            continue;
                        }
                        TRACE(m_csErrorMessage);
                        m_uFileState = SF_ABORT;
                        break;
                    }
                }
                if (!IsSilent())  {
                    AfxMessageBox(m_csErrorMessage, MB_ICONSTOP);
                }
                TRACE(m_csErrorMessage);
                m_uFileState = SF_ABORT;
            }
            else {
                bDone = true;
            }
        }
    }
    else {
        while (!bDone) {
             //if (!CStdioFile::Open(pszFileName, uOpenFlag | CFile::modeCreate | CFile::shareExclusive | CFile::typeText, m_pFileException)) {
            if (!CStdioFileUnicode::Open(pszFileName, uOpenFlag | CFile::modeCreate | CFile::shareExclusive | CFile::typeText, m_pFileException)) { // 20111215 for unicode
                bDone = true;
                CString csLoad;

                csLoad.LoadString(IDS_ERROROPENWRITE);
                m_csErrorMessage.Format (csLoad, pszFileName);
                if (m_pFileException->m_cause == CFileException::accessDenied) {
                    csLoad.LoadString(IDS_ACCESSDENIED);
                    m_csErrorMessage += csLoad;
                    if (!IsSilent())  {
                        if (AfxMessageBox(m_csErrorMessage, MB_RETRYCANCEL | MB_ICONEXCLAMATION) == IDRETRY) {
                            bDone = false;
                            continue;
                        }
                        TRACE(m_csErrorMessage);
                        m_uFileState = SF_ABORT;
                        break;
                    }
                }
                else if (m_pFileException->m_cause == CFileException::sharingViolation) {
                    csLoad.LoadString(IDS_INUSEBYANOTHER);
                    m_csErrorMessage += csLoad;
                    if (!IsSilent())  {
                        if (AfxMessageBox(m_csErrorMessage, MB_RETRYCANCEL | MB_ICONEXCLAMATION) == IDRETRY) {
                            bDone = false;
                            continue;
                        }
                        TRACE(m_csErrorMessage);
                        m_uFileState = SF_ABORT;
                        break;
                    }
                }
                if (!IsSilent())  {
                    AfxMessageBox(m_csErrorMessage, MB_ICONSTOP);
                }
                TRACE(m_csErrorMessage);
                m_uFileState = SF_ABORT;
            }
            else {
                bDone = true;
            }
        }
    }
    m_uLineNumber = 0;
    m_bPushBack = FALSE;
    return (m_uFileState == SF_OK);
}
#else
/*virtual*/ BOOL CSpecFile::Open (const csprochar* pszFileName, UINT uOpenFlag) {


    ASSERT (pszFileName != NULL);

    // 20100818 this assert was failing when being called in a tab application (due to additional flags)
    //ASSERT (uOpenFlag == CFile::modeRead || uOpenFlag == CFile::modeWrite);

    bool bDone = false;
    m_csFileName = pszFileName;
    m_uFileState = SF_OK;
    if (uOpenFlag == CFile::modeRead)
    {
        m_bIsReading = TRUE;
        while (!bDone) {
            if (!CStdioFileUnicode::Open(pszFileName, uOpenFlag | CFile::shareDenyWrite | CFile::typeText)) { // 20111215 for unicode, also changing type to text
                bDone = true;
                m_csErrorMessage.Format (_T("Could not open file: %s"), pszFileName);
                m_uFileState = SF_ABORT;
            }
            else {
                bDone = true;
            }
        }
    }
    else {
        while (!bDone) {
            if (!CStdioFileUnicode::Open(pszFileName, uOpenFlag | CFile::modeCreate | CFile::shareExclusive | CFile::typeText)) { // 20111215 for unicode
                bDone = true;
                m_csErrorMessage.Format (_T("Could not open file: %s"), pszFileName);
                m_uFileState = SF_ABORT;
            }
            else {
                bDone = true;
            }
        }
    }
    m_uLineNumber = 0;
    m_bPushBack = FALSE;
    return (m_uFileState == SF_OK);
}
#endif

/////////////////////////////////////////////////////////////////////////////
//
//                               CSpecFile::Close
//
/////////////////////////////////////////////////////////////////////////////
#ifdef WIN_DESKTOP
/*virtual*/ void CSpecFile::Close (void) {

    TRY {
        CStdioFile::Close();
    }
    CATCH(CFileException, e) {
        memcpy(m_pFileException, e, sizeof(m_pFileException));
        if (e->m_cause == CFileException::diskFull) {
            CString csLoad;
            csLoad.LoadString(IDS_DISKFULL);
            m_csErrorMessage.Format(csLoad, (LPCTSTR)m_csFileName);
        }
        else {
            CString csLoad;
            csLoad.LoadString(IDS_ERRORCLOSING);
            m_csErrorMessage.Format(csLoad, (LPCTSTR)m_csFileName);
        }
        if (!IsSilent())  {
            AfxMessageBox(m_csErrorMessage, MB_ICONSTOP);
        }
        TRACE(m_csErrorMessage);
        m_uFileState = SF_ABORT;
    }
    END_CATCH
}
#else
/*virtual*/ void CSpecFile::Close (void) {
    // call the base class member
    CStdioFile::Close();
}

#endif

/////////////////////////////////////////////////////////////////////////////
//
//                               CSpecFile::PutLine
//
/////////////////////////////////////////////////////////////////////////////

UINT CSpecFile::PutLine(const TCHAR* pszLine)
{
    ASSERT (!m_bIsReading);

    if (m_uFileState != SF_OK) {
        return m_uFileState;
    }

    BOOL bDone;
    int iLength = _tcslen(pszLine);
    if (iLength == 0) {
        Write(NEWLINE, 1);
        bDone = TRUE;
    }
    else {
        bDone = FALSE;
    }
    while (!bDone) {
        if (*(pszLine + iLength - 1) != AMPERSAND) {
            Write(pszLine, iLength);
            Write(NEWLINE, 1);
            bDone = TRUE;
            continue;
        }

        if (iLength == 1) {
            Write(pszLine, iLength);
            Write(NEWLINE, 1);
            bDone = TRUE;
            continue;
        }

        int iLen = iLength;
        while (iLen > 1 && *(pszLine + iLen - 1) == AMPERSAND) {
            iLen--;
        }
        Write(pszLine, iLen);
        Write(CONTLINE, 2);
        pszLine += iLen;
        iLength = _tcslen(pszLine);
        ASSERT(iLength > 0);
        m_uLineNumber++;
    }
    m_uLineNumber++;
    return (m_uFileState);
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CSpecFile::Write
//
/////////////////////////////////////////////////////////////////////////////
#ifdef WIN_DESKTOP
/*virtual*/ void CSpecFile::Write(const csprochar* pszText, UINT uSize) {

    TRY {

        //CStdioFile::Write(pszText, uSize);
        //ASSERT(!pszText[uSize]);
        // 20120821 the above assert was sometimes getting hit when CDEText was saving itself (with ampersands to indicate multiple lines)
        if( pszText[uSize] )
        {
            TCHAR tempChar = pszText[uSize];
            csprochar * pszTextNonConst = const_cast<csprochar *>(pszText);
            pszTextNonConst[uSize] = 0;
            CStdioFile::WriteString(pszTextNonConst);
            pszTextNonConst[uSize] = tempChar;
        }

        else
            CStdioFile::WriteString(pszText); // 20111212 for unicode
    }
    CATCH (CFileException, e) {
        memcpy(m_pFileException, e, sizeof(m_pFileException));
        if (e->m_cause == CFileException::diskFull) {
            CString csLoad;
            csLoad.LoadString(IDS_DISKFULL);
            m_csErrorMessage.Format(csLoad, (LPCTSTR)m_csFileName);
        }
        else {
            CString csLoad;
            csLoad.LoadString(IDS_ERRORWRITING);
            m_csErrorMessage.Format(csLoad, (LPCTSTR)m_csFileName);
        }
        if (!IsSilent())  {
            AfxMessageBox(m_csErrorMessage, MB_ICONSTOP);
        }
        TRACE(m_csErrorMessage);
        m_uFileState = SF_ABORT;
    }
    END_CATCH
}
#else
/*virtual*/ void CSpecFile::Write(const csprochar* pszText, UINT uSize) // 20131104
{
    std::string str = UTF8Convert::WideToUTF8(pszText,uSize);

    // to ensure correct output on windows systems, we need to write out the \r anytime there is a \n
    int startPos = 0,curPos = 0;
    bool keepProcessing = true;

    while( keepProcessing )
    {
        bool printString = false;
        bool newline = false;

        if( curPos >= str.length() || !str[curPos] ) // end of the string
        {
            printString = true;
            keepProcessing = false;
        }

        else if( str[curPos] == '\n' )
        {
            printString = true;
            newline = true;
        }

        if( printString )
        {
            if( startPos != curPos )
            {
                std::string partialStr = str.substr(startPos,curPos - startPos);
                CFile::Write(partialStr.c_str(),partialStr.length());
            }

            if( newline )
            {
#ifdef _CONSOLE
                CFile::Write("\n", 1);
#else
                CFile::Write("\r\n", 2);
#endif
            }

            startPos = curPos + 1;
        }

        curPos++;
    }
}
#endif

/////////////////////////////////////////////////////////////////////////////
//
//                               CSpecFile::GetLine
//
/////////////////////////////////////////////////////////////////////////////

// RHF Jul 10, 2002  Add bTrim=true
#ifdef WIN_DESKTOP
/*virtual*/ UINT CSpecFile::GetLine(CString& csAttribute,CString& csValue, bool bTrim/* = true*/) {

    if (m_uFileState != SF_OK) {
        return m_uFileState;
    }

    if (m_bPushBack) {
        m_bPushBack = FALSE;
        csAttribute = m_csAttribute;
        csValue = m_csValue;
        if (bTrim) {                   // BMD 21 Sep 2006
            csAttribute.Trim();
            csValue.Trim();
        }
        return m_uFileState;
    }
    BOOL bDone = FALSE;
    CString csLine, csPart;
    //csprochar* pszPart;
    while (!bDone) {
        BOOL bEOF = FALSE;
        BOOL bContinue = TRUE;

        do {
            TRY {
                bEOF = !ReadString(csPart);
            }
            CATCH (CFileException, e) {
                memcpy(m_pFileException, e, sizeof(m_pFileException));
                CString csLoad;
                csLoad.LoadString(IDS_ERRORREADING);
                m_csErrorMessage.Format(csLoad, (LPCTSTR)m_csFileName);
                if (!IsSilent())  {
                    AfxMessageBox(m_csErrorMessage, MB_ICONSTOP);
                }
                TRACE(m_csErrorMessage);
                m_uFileState = SF_ABORT;
            }
            END_CATCH
            int iLast;
            /*if (pszPart != NULL) {
                iLast = _tcslen(pszPart);
                if (iLast < 2) {                    // BMD 09 Apr 2003
                    *pszPart = _T('\0');
                }
                else {
                    *(pszPart + iLast - 2) = _T('\0');
                }
            }
            csPart.ReleaseBuffer(-1);*/
            if (m_uFileState != SF_OK) {
                return m_uFileState;
            }
            //if (pszPart == NULL)  {
            if( bEOF ) {
                m_uFileState = SF_EOF;
                return m_uFileState;
            }
            m_uLineNumber++;
            iLast = csPart.GetLength() - 1;
            if (iLast > 0 && csPart[iLast] == AMPERSAND) {
                csLine += csPart.Left(iLast);
            }
            else {
                csLine += csPart;
                bContinue = FALSE;
            }
        } while (bContinue);

        if( bTrim ) { // RHF Jul 10, 2002
            csLine.Trim();
        }// RHF Jul 10, 2002
        if (csLine.IsEmpty()) {
            continue;
        }

        // gsf 11/26 begin
        // treat dot in first position as comment
        // at least for now, for Serpro testing
        if (csLine.GetAt(0) == '.') {
            csLine =_T("");
            continue;
        }
        // gsf 11/26 end

        int iEqualPos = csLine.Find('=');
        if (iEqualPos == -1)  {
            csAttribute = csLine;
            m_csAttribute = csLine;
            csValue.Empty();
            m_csValue.Empty();
        }
        else {
            csAttribute = csLine.Left(iEqualPos);
            if( bTrim ) // RHF Jul 10, 2002
                csAttribute.TrimRight();
            m_csAttribute = csAttribute;
            csValue = csLine.Mid(iEqualPos + 1);
            if( bTrim ) // RHF Jul 10, 2002
                csValue.TrimLeft();
            m_csValue = csValue;
        }
        bDone = TRUE;
    }
    return m_uFileState;
}
#else
/*virtual*/ UINT CSpecFile::GetLine(CString& csAttribute,CString& csValue, bool bTrim ) {

    if (m_uFileState != SF_OK) {
        return m_uFileState;
    }

    if (m_bPushBack) {
        m_bPushBack = FALSE;
        csAttribute = m_csAttribute;
        csValue = m_csValue;
        if (bTrim) {                   // BMD 21 Sep 2006
            csAttribute.Trim();
            csValue.Trim();
        }
        return m_uFileState;
    }
    BOOL bDone = FALSE;
    CString csLine, csPart;
    //csprochar* pszPart;
    while (!bDone) {
        BOOL bEOF;

        BOOL bContinue = TRUE;
        do {
            bEOF = !ReadString(csPart);
            if (m_uFileState != SF_OK) {
                return m_uFileState;
            }
            //if (pszPart == NULL)  {
            if( bEOF ) {
                m_uFileState = SF_EOF;
                return m_uFileState;
            }
            m_uLineNumber++;
            int iLast = csPart.GetLength() - 1;
            if (iLast > 0 && csPart[iLast] == AMPERSAND) {
                csLine += csPart.Left(iLast);
            }
            else {
                csLine += csPart;
                bContinue = FALSE;
            }
        } while (bContinue);

        if( bTrim ) { // RHF Jul 10, 2002
            csLine.TrimRight();
            csLine.TrimLeft();
        }// RHF Jul 10, 2002
        if (csLine.IsEmpty()) {
            continue;
        }

        // gsf 11/26 begin
        // treat dot in first position as comment
        // at least for now, for Serpro testing
        if (csLine.GetAt(0) == '.') {
            csLine =_T("");
            continue;
        }
        // gsf 11/26 end

        int iEqualPos = csLine.Find('=');
        if (iEqualPos == -1)  {
            csAttribute = csLine;
            m_csAttribute = csLine;
            csValue.Empty();
            m_csValue.Empty();
        }
        else {
            csAttribute = csLine.Left(iEqualPos);
            if( bTrim ) // RHF Jul 10, 2002
                csAttribute.TrimRight();
            m_csAttribute = csAttribute;
            csValue = csLine.Mid(iEqualPos + 1);
            if( bTrim ) // RHF Jul 10, 2002
                csValue.TrimLeft();
            m_csValue = csValue;
        }
        bDone = TRUE;
    }
    return m_uFileState;
}
#endif
/////////////////////////////////////////////////////////////////////////////
//
//                               CSpecFile::UngetLine
//
/////////////////////////////////////////////////////////////////////////////

void CSpecFile::UngetLine () {

    ASSERT (!m_bPushBack);
    m_bPushBack = TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//
//                               CSpecFile::SkipSection
//
/////////////////////////////////////////////////////////////////////////////

UINT CSpecFile::SkipSection(void) {

    CString csAttribute;
    CString csValue;
    BOOL bDone = FALSE;
    while (!bDone) {
        if (GetLine(csAttribute, csValue) == SF_OK) {
            if (csAttribute[0] == '[')  {
                bDone = TRUE;
                UngetLine();
            }
        }
        else {
            bDone = TRUE;
        }
    }
    return (m_uFileState);
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CSpecFile::IsHeaderOK
//
/////////////////////////////////////////////////////////////////////////////

BOOL CSpecFile::IsHeaderOK(const CString& csType)  {

    CString csAttribute;
    CString csValue;
    if (GetLine(csAttribute, csValue) != SF_OK) {
        return FALSE;
    }
    if (csAttribute.CompareNoCase(csType) != 0) {
        return FALSE;
    }
    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CSpecFile::IsVersionOK
//
/////////////////////////////////////////////////////////////////////////////

BOOL CSpecFile::IsVersionOK(const CString& csVersion, double* out_version_number/* = nullptr*/) {
    CString csAttribute;
    CString csValue;
    if (GetLine(csAttribute, csValue) != SF_OK) {
        return FALSE;
    }
    if (csAttribute.CompareNoCase(CMD_VERSION) != 0) {
        return FALSE;
    }
    /*if (csValue.CompareNoCase(csVersion) != 0) {
        return FALSE;
    }*/
    // 20130226 .sts files created in previous versions of CSPro were not being recognized
    if( csVersion.CompareNoCase(csValue) >= 0 ) {
        if( out_version_number != nullptr )
            *out_version_number = GetCSProVersionNumeric(csValue);
        return TRUE;
    }

    return FALSE;
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CSpecFile::IsVersionOK
//
/////////////////////////////////////////////////////////////////////////////

BOOL CSpecFile::IsVersionOK(CString& csVersion)  {

    CString csAttribute;
    CString csValue;
    if (GetLine(csAttribute, csValue) != SF_OK) {
        csVersion =_T("Bad");
        return FALSE;
    }
    if (csAttribute.CompareNoCase(CMD_VERSION) != 0) {
        csVersion =_T("Bad");
        return FALSE;
    }
    csVersion = csValue;
    if (csVersion.CompareNoCase(CSPRO_VERSION) != 0) {
        return FALSE;
    }
    return TRUE;
}


// JH 12/29/05
// Workaround for bug in CFile::GetFilePath which fails due to a bug in MFC (CFile::GetStatus bug) when
// reading from CDROM.
CString CSpecFile::GetFilePathSafe() const
{
    ASSERT_VALID(this);

    // from the source code for CFile::GetFilePath and CFile::GetStatus it looks to me
    // like this all that happens, GetFilePath calls GetStatus and then returns the path
    // from the CFileStatus structure.  GetStatus just copies the path from the
    // m_strFileName variable.
    return m_strFileName;
}


CString CSpecFile::EvaluateRelativeFilename(const CString& filename) const
{
    // old spec files serialized filenames like this:
    // File=7/13/2006 1:12:08 PM,.\GeoFile.dcf

    // this method will convert the filename to an absolute filename and will,
    // based on the presence of a comma and the existence of a file, determine
    // whether the comma should be part of the the filename
    int comma_pos = filename.Find(_T(','));
    CString directory_name = PortableFunctions::PathGetDirectory<CString>(m_csFileName);
    CString absolute_filename = WS2CS(MakeFullPath(directory_name, CS2WS(filename)));

    if( comma_pos >= 0 && !PortableFunctions::FileIsRegular(absolute_filename) )
    {
        CString absolute_filename_skipping_comma = WS2CS(MakeFullPath(directory_name, CS2WS(filename.Mid(comma_pos + 1))));

        if( PortableFunctions::FileIsRegular(absolute_filename_skipping_comma) )
            return absolute_filename_skipping_comma;
    }

    return absolute_filename;
}
