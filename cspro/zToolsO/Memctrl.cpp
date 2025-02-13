#include "StdAfx.h"
#include "Tools.h"
#include <stdlib.h>


// traza references removed on 2019-01-09 ... if using this functionality, you may want to
// look at changeset 8563 and bring back some removed behavior

extern "C" {
CLASS_DECL_ZTOOLSO void *icalloc(csprochar* pszFileName, int iLine, size_t iSize, size_t iLen );
CLASS_DECL_ZTOOLSO void *imalloc(csprochar* pszFileName, int iLine, size_t iSize);
CLASS_DECL_ZTOOLSO void ifree(csprochar* pszFileName, int iLine, void *p );
CLASS_DECL_ZTOOLSO void ifree(csprochar* pszFileName, int iLine, void *p );

CLASS_DECL_ZTOOLSO void *icalloc2(int iVal, int iLine, size_t iSize, size_t iLen ) ;
CLASS_DECL_ZTOOLSO void ifree2(int iVal, int iLine, void* ptr );
CLASS_DECL_ZTOOLSO void *imalloc2(int iVal, int iLine, size_t iSize );
};


class CMemoryDump
{
private:
    CString m_csPrefix;
    bool    m_bFreeUnUsed;
    bool    m_bDumped;

public:
    static  int m_iCount;

    bool    m_bTrace;
    CString m_csMsg;
    CString m_csOutFileName;

    std::map<void*, CString> MemMap;

    CMemoryDump();
    ~CMemoryDump();

    void Dump();
};


static CMemoryDump MemDump;
int CMemoryDump::m_iCount=0;
static CMemoryDump* pMemDump=NULL;


static void SetMemDump( CMemoryDump* pMem ) {
    pMemDump = pMem;
}

#ifndef WIN32
void* icalloc2(int iVal, int iLine, size_t iSize, size_t iLen )
{
    void*  p=calloc( iSize, iLen );

    ASSERT( pMemDump );
    pMemDump->m_csMsg = _T("") ;
    pMemDump->MemMap[p] = pMemDump->m_csMsg;
    return( p );
}
void *imalloc2(int iVal, int iLine, size_t iSize )
{
    void*  p=malloc( iSize );

    ASSERT( pMemDump );
    pMemDump->m_csMsg = _T("") ;
    pMemDump->MemMap[p] = pMemDump->m_csMsg;
    return( p );
}

void ifree2(int iVal, int iLine, void* ptr ) {
    ASSERT( pMemDump );

    const auto& lookup  = pMemDump->MemMap.find(ptr);
    if( lookup != pMemDump->MemMap.cend() ) {
        free(ptr);
        pMemDump->MemMap.erase(lookup);
    }
}

#endif

CLASS_DECL_ZTOOLSO void * icalloc(csprochar* pszFileName, int iLine, size_t iSize, size_t iLen ) {
    void*  p=calloc( iSize, iLen );

    ASSERT( pMemDump );
    if( pMemDump->m_bTrace ) {
        pMemDump->m_csMsg.Format( _T("CALLOC: (%s)(%05d) ptr=%x, size=%d, len=%d") , pszFileName, iLine, p, iSize, iLen );
    }
    else
        pMemDump->m_csMsg = _T("") ;

    pMemDump->MemMap[p] = pMemDump->m_csMsg;
    return( p );
}

CLASS_DECL_ZTOOLSO void * imalloc(csprochar* pszFileName, int iLine, size_t iSize) {
    void*  p=malloc( iSize );

    ASSERT( pMemDump );
    if( pMemDump->m_bTrace ) {
        pMemDump->m_csMsg.Format( _T("MALLOC: (%s)(%05d) ptr=%x, size=%d") , pszFileName, iLine, p, iSize );
    }
    else
        pMemDump->m_csMsg = _T("") ;

    pMemDump->MemMap[p] = pMemDump->m_csMsg;
    return( p );
}

CLASS_DECL_ZTOOLSO void ifree(csprochar* /*pszFileName*/, int /*iLine*/, void *p ) {
    ASSERT( pMemDump );

    const auto& lookup  = pMemDump->MemMap.find(p);
    if( lookup != pMemDump->MemMap.cend() ) {
        free(p);
        pMemDump->MemMap.erase(lookup);
    }
}



CMemoryDump::CMemoryDump() {
   m_bDumped = false;

   ASSERT( m_iCount == 0 );
   m_iCount++;

   SetMemDump( this );

   m_csPrefix.Format( _T("UNFREE:")  );
   m_csOutFileName = _T("$MEMORY$.LOG");

// m_bFreeUnUsed = (getenv(_T("FREE_GARBAGE") ) != NULL); // free unused
   m_bFreeUnUsed = true;
#ifdef WIN_DESKTOP
   m_bTrace = (_tgetenv(_T("TRACE_GARBAGE") ) != NULL);
#endif
   //m_bTrace = true;
}

CMemoryDump::~CMemoryDump() {
    if( !m_bTrace && !m_bFreeUnUsed )
        return;

    Dump();
}

void CMemoryDump::Dump() {
    if( m_bDumped )
        return;
    m_bDumped = true;
#ifdef _MFC_VER

    for( const auto& [p, csMsg] : MemMap ) {
        m_csMsg = csMsg;
        if( m_bTrace ) {
            m_csMsg = m_csPrefix + m_csMsg;
        }
        if( m_bFreeUnUsed )
            free( p );
    }

#endif
}
