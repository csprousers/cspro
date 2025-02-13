#pragma once

//---------------------------------------------------------------------------
//  File name: TbiFile.h
//
//  Description:
//          Header for CTbiFile class
//          This class is intended define the tbd index file
//
//  History:    Date       Author   Comment
//              ---------------------------
//              3 Jul 01   DVB      Created
//
//---------------------------------------------------------------------------

class SimpleDbMap;
#include <ZTBDO/zTbdO.h>

class CLASS_DECL_ZTBDO CTbiFile {
    SimpleDbMap* m_pTableIndex;
    CString     m_csFileName;
    int         m_iRegLen;

    // for iterating through the index
    std::wstring m_csKey;
    long m_lValue;

public:
    enum    CTbiFile_LocateMode { First, Next, Exact };

    CTbiFile();
    CTbiFile(CString csFileName, int iRegLen);
    void Init();

    virtual ~CTbiFile();

    // bRefKeyPrefix can be used optionally if eMoveMode is Exact
    bool Locate(CTbiFile_LocateMode eLocateMode, CString* pcsRefKeyPrefix = NULL, int* piRefKeyLen = NULL );

    void SetFileName( CString csFileName, int iRegLen );
    CString GetFileName() { return m_csFileName; }

    bool Open( bool bCreate );  // Open Tbi
    bool Close();               // Close Tbi

    long  GetCurrentOffset() const     { return m_lValue; }
    const TCHAR* GetCurrentReg() const { return m_csKey.c_str(); }

    int  GetRegLen() { return m_iRegLen; }
};
