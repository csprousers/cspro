#include "StdAfx.h"
#include "TbdFileM.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#define new DEBUG_NEW
#endif

//-----------------------------
// Method: Constructor
//-----------------------------
CTbdFileMgr::CTbdFileMgr()
{
}

//-----------------------------
// Method: Destructor
//-----------------------------
CTbdFileMgr::~CTbdFileMgr() {
    for( int i=0; i < m_aTbdFiles.GetSize(); i++ )
        delete m_aTbdFiles.ElementAt(i);
}

//-----------------------------
// Method: Open
//-----------------------------
bool CTbdFileMgr::Open( CString csFileName, bool bCreate) {
    CTbdFile *ptFile;

    // If filename was open before then we must return true
    for ( int iPos = 0; iPos < m_aTbdFiles.GetSize() ; iPos++ )
        if ( m_aTbdFiles.GetAt(iPos)->GetFileName() == csFileName )
            return true;

    ptFile = new CTbdFile(csFileName);


    if ( !bCreate ) {
        if ( !ptFile->Open(bCreate) ) {
            ptFile->Close();
            delete ptFile;
            return false;
        }
    }
    else {
        if ( !ptFile->DoOpen(bCreate) ) {
            ptFile->Close();
            delete ptFile;
            return false;
        }
    }

    m_aTbdFiles.Add(ptFile);

    return true;
}

//---------------------------------------
// Method: Close
//
// Description: When a file is closed,
//              this is removed from
//              m_aTbdFiles.
//---------------------------------------
bool CTbdFileMgr::Close( CString csFileName ) {

    for ( int i = 0; i < m_aTbdFiles.GetSize(); i++)
        if ( m_aTbdFiles.GetAt(i)->GetFileName() == csFileName ) {
            m_aTbdFiles.GetAt(i)->Close();
            m_aTbdFiles.RemoveAt(i);
            return true;
        }

    return false;
}

//-----------------------------
// Method: CloseAll
//-----------------------------
bool CTbdFileMgr::CloseAll() {

    for ( int i = 0; i < m_aTbdFiles.GetSize(); i++) {
        m_aTbdFiles.GetAt(i)->Close();
    }

    return true;
}

//-----------------------------
// Method: FlushAll
//-----------------------------
bool CTbdFileMgr::FlushAll() {

    for ( int i = 0; i < m_aTbdFiles.GetSize(); i++) {
        m_aTbdFiles.GetAt(i)->Flush();
    }

    return true;
}


//-----------------------------
// Method: GetTbdFile
//-----------------------------
CTbdFile*  CTbdFileMgr::GetTbdFile(CString csFileName ) {

    for ( int i = 0; i < m_aTbdFiles.GetSize(); i++ )
        if (m_aTbdFiles.GetAt(i)->GetFileName() == csFileName)
            return m_aTbdFiles.GetAt(i);

    return NULL;
}

