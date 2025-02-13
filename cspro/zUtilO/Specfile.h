#pragma once

//***************************************************************************
//  File name: SPECFILE.H
//
//  Description:
//       Header for CSpecFile class.
//
//  History:    Date       Author   Comment
//              ---------------------------
//              21 Jan 98   BMD     Created from IMPS 4.1
//
//***************************************************************************
//
//  class CSpecFile : public CStdioFileUnicode
//
//  Description:
//      Add functionality to hand IMSA specification files.
//
//  Construction
//      CSpecFile               Special constructor to run silently.
//      Open                    Open a spec file.
//      Close                   Close a spec file.
//
//  Input/Output
//      GetLine                 Get next line from the file a break into attribute and value.
//      GetLineNumber           Get number of line in file
//      UngetLine               Put the last gotten line back onto the file.
//      SkipSection             Skip to the next section heading.
//      PutLine                 Put line to spec file.
//      SetSilent               Set to operate silently.
//
//  Status
//      GetState                Get state of file (OK, Abort, EOF).
//      SetState                Set state of file.
//      IsReading               Is the file open for reading.
//      IsSilent                Is silent operation set.
//      IsHeaderOK              Is file header OK.
//      IsVersionOK             Is file version OK.
//      GetFileException        Get last file exception.
//
//***************************************************************************
//***************************************************************************
//
//  CSpecFile::CSpecFile(BOOL bIsSilent = FALSE);
//
//      Parameters
//          bIsSilent           TRUE to suppress error messages.
//                              FALSE to display error messages.
//
//---------------------------------------------------------------------------
//
//  virtual BOOL CSpecFile::Open(const csprochar* pszFileName, UINT uOpenFlags);
//
//      Parameters
//          pszFileName         Full name of spec file to be opened.
//          uOpenFlags          CFile::modeRead         Open for reading.
//                              CFile::modeReadWrite    Open for reading and writing the file.
//                              CFile::modeWrite        Open for writing the file.
//      Result value
//          TRUE if file opened successfully, otherwise FALSE.
//
//---------------------------------------------------------------------------
//
//  virtual void CSpecFile::Close(void);
//
//      Remarks
//          Use to close file and clean up.
//
//***************************************************************************
//
//  virtual UINT CSpecFile::GetLine(CString& csAttribute, CString& csValue);
//
//      Parameters
//          csAttribute         String to receive the section headings or attribute name read
//          csValue             String to receive the value of an attribute read.
//
//      Result value
//          State of the file.
//              SF_OK           Operation successful.
//              SF_ABORT        Operation failed, I/O error.
//              SF_EOF          End of file reached.
//
//      Remarks
//          Blank lines are ignored.  The next non-blank line is returned separated into
//          attribute and value.  The attribute and value strings are trimmed both left and right.
//
//---------------------------------------------------------------------------
//
//  UINT CSpecFile::GetLineNumber(void) const;
//
//      Return value
//          The number of the last line retrieived by GetLine().
//
//      Remarks
//          Counts blank line, even thought they are ignored by GetLine().
//
//---------------------------------------------------------------------------
//
//  void CSpecFile::UngetLine (void);
//
//      Remarks
//          Stores the last attribute/value pair retrieved using GetLine(),
//          so that the next call to GetLine() will retrieve them.
//          Works only for the last line gotten using GetLine().
//
//---------------------------------------------------------------------------
//
//  UINT CSpecFile::SkipSection(void);
//
//      Return value
//          State of the file.
//              SF_OK           Operation successful.
//              SF_ABORT        Operation failed, I/O error.
//              SF_EOF          End of file reached.
//
//      Remarks
//          Scans to the next section header, entry beginning with left bracket ([).
//          The next call to GetLine() will get the section heading.
//          If there are no more sections, function returns SF_EOF.
//
//---------------------------------------------------------------------------
//
//  virtual UINT CSpecFile::PutLine(const CString& csLine);
//
//      Return value
//          State of the file.
//              SF_OK           Operation successful.
//              SF_ABORT        Operation failed, I/O error.
//              SF_EOF          End of file reached.
//
//---------------------------------------------------------------------------
//
//  UINT CSpecFile::PutLine(const CString& csAttribute, const CString& csValue);
//
//      Return value
//          State of the file.
//              SF_OK           Operation successful.
//              SF_ABORT        Operation failed, I/O error.
//              SF_EOF          End of file reached.
//
//      Remarks
//          Writes csAttribute + "=" + csValue to the spec file.
//
//---------------------------------------------------------------------------
//
//  void CSpecFile::SetSilent(BOOL bIsSilent);
//
//      Parameters
//          bIsSilent           TRUE to set silent operation
//                              FALSE to display error messages
//
//***************************************************************************
//
//  UINT CSpecFile::GetState(void) const;
//
//      Return value
//          State of the file.
//              SF_OK           Operation successful.
//              SF_ABORT        Operation failed, I/O error.
//              SF_EOF          End of file reached.
//
//---------------------------------------------------------------------------
//
//  void CSpecFile::SetState(UINT uFileState);
//
//      Parameters
//          uFileState          SF_OK       File OK.
//                              SF_ABORT    Bad I/O operation, fatal error.
//                              SF_EOF      At end of file.
//
//      Remarks
//          Used when file is repositioned.
//
//---------------------------------------------------------------------------
//
//  BOOL CSpecFile::IsReading(void) const;
//
//      Return value
//          TRUE if file is opened for reading only, otherwise FALSE.
//
//---------------------------------------------------------------------------
//
//  BOOL CSpecFile::IsSilent(void) const;
//
//      Return value
//          TRUE if file is set for silent operation, otherwise FALSE.
//
//---------------------------------------------------------------------------
//
//  BOOL CSpecFile::IsHeaderOK(const CString& csType);
//
//      Parmeters
//          csType              Header string which the file should have.
//
//      Return value
//          TRUE if file has the given header, otherwise FALSE.
//
//---------------------------------------------------------------------------
//
//  BOOL CSpecFile::IsVersionOK(const CString& csVersion);
//
//      Parmeters
//          csVersion           Version string which the file should have.
//
//      Return value
//          TRUE if file has the given version, otherwise FALSE.
//
//---------------------------------------------------------------------------
//
//  BOOL CSpecFile::IsVersionOK(CString& csVersion);
//
//      Parmeters
//          csVersion           Version string which file has is returned here.
//
//      Return value
//          TRUE if file has the given version, otherwise FALSE.
//
//---------------------------------------------------------------------------
//
//  CFileException* CSpecFile::GetFileException(void) const;
//
//      Return value
//          Pointer to the CFileException of the last I/O operation
//
//      Remarks
//          Used to find the cause of an I/O error
//
//***************************************************************************

#define SF_OK        0
#define SF_EOF       2
#define SF_ABORT     3

#define AMPERSAND   _T('&')
#define NEWLINE     _T("\n")
#define CONTLINE    _T("&\n")


/////////////////////////////////////////////////////////////////////////////
//
//                               CSpecFile
//
/////////////////////////////////////////////////////////////////////////////

// GHM 20111215 changed parent class from CStdioFile to CStdioFileUnicode

#include <zUtilO/zUtilO.h>
#include <zUtilO/Interapp.h>
#include <zUtilO/StdioFileUnicode.h>


class CLASS_DECL_ZUTILO CSpecFile : public CStdioFileUnicode
{
// Methods
public:
// Construction
    CSpecFile(BOOL bIsSilent=FALSE);
    ~CSpecFile(void);

    virtual BOOL Open(const csprochar* pszFileName, UINT uOpenFlags);

    virtual void Close(void);


// Input/Output
    virtual UINT GetLine(CString& csAttribute,CString& csValue, bool bTrim=true ); // RHF Jul 10, 2002 Add bTrim=true
    UINT GetLineNumber(void) const { return m_uLineNumber; }
    void UngetLine (void);
    UINT SkipSection(void);

    UINT PutLine(const TCHAR* pszLine);

    UINT PutLine(NullTerminatedString attribute, NullTerminatedString value) { return PutLine(FormatText(_T("%s=%s"), attribute.c_str(), value.c_str())); }
    UINT PutLine(NullTerminatedString attribute, int32_t value)              { return PutLine(std::move(attribute), IntToString(value)); }
    UINT PutLine(NullTerminatedString attribute, uint32_t value)             { return PutLine(std::move(attribute), IntToString(value)); }
#ifdef WASM
    UINT PutLine(NullTerminatedString attribute, unsigned long value)        { return PutLine(std::move(attribute), IntToString(value)); }
#endif
    UINT PutLine(NullTerminatedString attribute, int64_t value)              { return PutLine(std::move(attribute), IntToString(value)); }
    UINT PutLine(NullTerminatedString attribute, uint64_t value)             { return PutLine(std::move(attribute), IntToString(value)); }
    UINT PutLine(NullTerminatedString attribute, double value)               { return PutLine(std::move(attribute), FormatText(_T("%f"), value)); }

    void SetSilent(BOOL bIsSilent)  { m_bIsSilent = bIsSilent; }

// Status
    UINT GetState(void) const { return m_uFileState; }
    void SetState(UINT uFileState) { m_uFileState = uFileState;}
    BOOL IsReading(void) const { return (m_bIsReading); }
    BOOL IsSilent(void) const { return m_bIsSilent; }
    BOOL IsHeaderOK(const CString& csType);
    BOOL IsVersionOK(const CString& csVersion, double* out_version_number = nullptr);
    BOOL IsVersionOK(CString& csVersion);

#ifdef WIN_DESKTOP
    CFileException* GetFileException(void) const { return m_pFileException; }
#endif

public:
    virtual void Write(const csprochar* pszText, UINT usize);

    // JH 12/29/05
    // Workaround for bug in CFile::GetFilePath which fails due to a bug in MFC (CFile::GetStatus bug) when
    // reading from CDROM.
    CString GetFilePathSafe() const;

    CString EvaluateRelativeFilename(const CString& filename) const;

protected:
    BOOL                m_bIsReading;
    BOOL                m_bPushBack;
    BOOL                m_bIsSilent;
    UINT                m_uLineNumber;
    UINT                m_uFileState;
#ifdef WIN_DESKTOP
    CFileException*     m_pFileException;
#endif
    CString             m_csAttribute;
    CString             m_csValue;
    CString             m_csFileName;
    CString             m_csErrorMessage;
};



inline const TCHAR* BOOL_TO_TEXT(bool bool_value)
{
    return bool_value ? CSPRO_ARG_YES : CSPRO_ARG_NO;
}

inline bool TEXT_TO_BOOL(const TCHAR* string_value)
{
    return ( _tcsicmp(string_value, CSPRO_ARG_YES) == 0 );
}
