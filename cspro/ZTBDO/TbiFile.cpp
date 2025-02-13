#include "StdAfx.h"
#include "TbiFile.h"
#include <zToolsO/PortableFunctions.h>
#include <zUtilO/SimpleDbMap.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#define new DEBUG_NEW
#endif


//-------------------------
// Method: Constructor
//-------------------------
CTbiFile::CTbiFile() {
    Init();
}

//-------------------------
// Method: Destructor
//-------------------------
CTbiFile::~CTbiFile() {
    Close();
}

void CTbiFile::SetFileName( CString csFileName, int iRegLen ) {
    m_csFileName = csFileName;
    m_iRegLen = iRegLen;
}

//-------------------------
// Method: Constructor
//-------------------------
CTbiFile::CTbiFile(CString csFileName, int iRegLen) {
    Init();
    m_csFileName = csFileName;
    m_iRegLen = iRegLen;
}

void CTbiFile::Init() {
    m_csFileName = _T("");
    m_iRegLen = -1;
    m_pTableIndex = NULL;
    m_csKey = _T("");
    m_lValue = 0;
}

//-------------------------
// Method: Open
//-------------------------
bool CTbiFile::Open(bool bCreate) {
    ASSERT( m_pTableIndex == NULL );

    if( !bCreate && !PortableFunctions::FileIsRegular(m_csFileName) )
        return false;

    m_pTableIndex = new SimpleDbMap();

    if( m_pTableIndex->Open((LPCTSTR)m_csFileName, { { _T("TBI"), SimpleDbMap::ValueType::Long } }) )
        return true;

    else
    {
        delete m_pTableIndex;
        m_pTableIndex = NULL;
        return false;
    }
}

//-------------------------
// Method: Close
//-------------------------
bool CTbiFile::Close() {
    if( m_pTableIndex != NULL )
    {
        delete m_pTableIndex;
        m_pTableIndex = NULL;
        return true;
    }
    else
        return false;
}


//-------------------------
// Method: Locate
//-------------------------
bool CTbiFile::Locate(CTbiFile_LocateMode eLocateMode, CString* pcsRefKeyPrefix, int* piRefKeyLen )
{
    ASSERT(m_pTableIndex != NULL);

    if( eLocateMode != Exact )
        ASSERT(pcsRefKeyPrefix == NULL && piRefKeyLen == NULL);

    if( ( piRefKeyLen != NULL ) && ( *piRefKeyLen > m_iRegLen ) )
        return false;

    if( eLocateMode == First )
    {
        m_pTableIndex->ResetIterator();
        return true;
    }

    else if( eLocateMode == Next )
    {
        return m_pTableIndex->NextLong(&m_csKey, &m_lValue);
    }

    else if( eLocateMode == Exact )
    {
        ASSERT(pcsRefKeyPrefix != NULL);

        std::optional<long> value = m_pTableIndex->GetLongUsingKeyPrefix(*pcsRefKeyPrefix);

        if( value.has_value() )
            m_lValue = *value;

        return value.has_value();
    }

    else
    {
        ASSERT(0);
        return false;
    }
}
