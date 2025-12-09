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
bool CTbiFile::Open(bool bCreate)
{
    ASSERT( m_pTableIndex == NULL );

    if( !bCreate && !PortableFunctions::FileIsRegular(m_csFileName) )
        return false;

    m_pTableIndex = std::make_unique<SimpleDbMap>();

    if( m_pTableIndex->Open(UTF8_TODO::GetUtf8(m_csFileName), { { "TBI", SimpleDbMap::ValueType::Long } }) )
        return true;

    m_pTableIndex.reset();
    return false;
}

//-------------------------
// Method: Close
//-------------------------
void CTbiFile::Close()
{
    m_pTableIndex.reset();
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
        std::string utf8_key;

        if( m_pTableIndex->NextLong(utf8_key, m_lValue) )
        {
            m_csKey = UTF8_TODO::GetWide(utf8_key);
            return true;
        }

        return false;
    }

    else if( eLocateMode == Exact )
    {
        ASSERT(pcsRefKeyPrefix != NULL);
        m_csKey = *pcsRefKeyPrefix; //sets the GetCurrentReg

        const std::optional<long> value = m_pTableIndex->GetLongUsingKeyPrefix(UTF8_TODO::GetUtf8(*pcsRefKeyPrefix));

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
