//---------------------------------------------------------------------------
//  File name: DicT.cpp
//
//  Description:
//          Implementation for engine-Dic class
//
//  History:    Date       Author   Comment
//              ---------------------------
//              27 Nov 99   RHF     Basic conversion
//              03 Dec 99   vc      Customization
//              24 Feb 00   RHF+vc  Adding WriteCaseBehavior
//
//
//---------------------------------------------------------------------------
#include "StdAfx.h"
#include "CFlow.h"
#include <engine/Tables.h>
#include <engine/Engdrv.h>
#include <engine/IntDrive.h>
#include <zToolsO/Serializer.h>
#include <zJson/Json.h>
#include <zCaseO/Case.h>


/////////////////////////////////////////////////////////////////////////////
//
// --- construction/destruction
//
/////////////////////////////////////////////////////////////////////////////


CSymbolDict::CSymbolDict(std::wstring name, CEngineDriver* pEngineDriver)
    :   Symbol(std::move(name), SymbolType::Pre80Dictionary),
        m_pEngineDriver(pEngineDriver),
        m_pEngineArea(pEngineDriver->getEngineAreaPtr())
{
    // dict' contents
    //TODO Fill when insert in symbol tables dp->SYMTowner = symtindx;
    SetFirstSymSec( -1 );
    SetLastSymSec( -1 );

    // descriptors for Level & Sect' identifiers
    for( int iLevel = 0; iLevel < (int)MaxNumberLevels; iLevel++ ) {
        qloc[iLevel] = 0;
        qlen[iLevel] = 0;
    }

    SetNumDim(0);

    SetMaxLevel( 0 );
    SetLevelsIdLen( 0 );
    SetSecIdLoc( 0 );// RHF Apr 27, 2000
    SetSecIdLen( 0 );

    m_iDictionaryAccessFlags = 0;

    // flow owner
    SetFlow( NULL );                // Flow owning this Dict

    // IMSA link
    m_pDataDict = NULL;
}


CSymbolDict::~CSymbolDict()
{
    Common_End();
}


const Logic::SymbolTable& CSymbolDict::GetSymbolTable() const
{
    return m_pEngineArea->GetSymbolTable();
}


void CSymbolDict::SetDataDict(std::shared_ptr<const CDataDict> dictionary)
{
    ASSERT(m_pDataDict == nullptr && dictionary != nullptr);

    m_pDataDict = std::move(dictionary);
    m_caseAccess = std::make_shared<CaseAccess>(*m_pDataDict);
}


/////////////////////////////////////////////////////////////////////////////
//
// --- dict keys (basic - primary key only)
//
/////////////////////////////////////////////////////////////////////////////
void CSymbolDict::ResetPrimaryKey() {
    m_PrimaryKeyInitial = _T("");
    m_PrimaryKeyCurrent = _T("");
}

void CSymbolDict::SetPrimaryKey( bool bCurrent ) {
    TCHAR pszKey[512];

    int iKeyLen = DictDoMainKey( pszKey, NULL, 1, 1 );
    pszKey[iKeyLen] = 0;

    if( bCurrent )                  // at-any-time contents
        m_PrimaryKeyCurrent = pszKey;
    else                            // initial contents
        m_PrimaryKeyInitial = pszKey;
}

void CSymbolDict::GetPrimaryKey(TCHAR* pszKey, bool bCurrent) {
    if( bCurrent )                  // at-any-time contents
        _stprintf( pszKey, _T("%ls"), m_PrimaryKeyCurrent.GetString() );
    else                            // initial contents
        _stprintf( pszKey, _T("%ls"), m_PrimaryKeyInitial.GetString() );
}


Symbol* CSymbolDict::FindChildSymbol(const std::wstring& symbol_name) const
{
    // get all the symbols matching this name and then check if any belong to this dictionary
    for( Symbol* symbol : GetSymbolTable().FindSymbols(symbol_name) )
    {
        if( symbol != this && SymbolCalculator(GetSymbolTable()).GetDicT(*symbol) == this )
            return symbol;
    }

    return nullptr;
}


void CSymbolDict::serialize_subclass(Serializer& ar)
{
    if( m_caseAccess != nullptr )
        ar & *m_caseAccess;

    ar & m_iDictionaryAccessFlags;
}


void CSymbolDict::WriteJsonMetadata_subclass(JsonWriter& json_writer) const
{
    ASSERT(m_pDataDict != nullptr);

    json_writer.Write(JK::subtype, _T("Dictionary"))
               .Write(JK::dictionaryType, GetSubType())
               .Write(JK::dictionary, *m_pDataDict);
}


void CSymbolDict::WriteValueToJson(JsonWriter& json_writer) const
{
    ASSERT(m_pDicX != nullptr && m_caseAccess != nullptr);
    ASSERT(m_pEngineDriver != nullptr && m_pEngineDriver->m_pIntDriver != nullptr);

    json_writer.BeginObject();

    // case
    if( m_pEngineDriver->m_pIntDriver->IsDataAccessible(*this, false) )
    {
        std::unique_ptr<Case> data_case = m_caseAccess->CreateCase();

        m_pEngineDriver->PrepareCaseFromEngineForQuestionnaireViewer(const_cast<DICT*>(this), *data_case);

        json_writer.Write(JK::case_, *data_case);
    }

    // dataSource
    {
        const DataRepository& data_repository = m_pDicX->GetDataRepository();

        json_writer.BeginObject(JK::dataSource)
                   .Write(JK::type, ToString(data_repository.GetRepositoryType()))
                   .Write(JK::connectionString, data_repository.GetConnectionString())
                   .EndObject();
    }

    json_writer.EndObject();
}
