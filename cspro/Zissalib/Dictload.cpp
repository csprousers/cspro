#include "StdAfx.h"
#include <zEngineO/DeprecatedSymbol.h>
#include <zEngineO/EngineItem.h>
#include <zEngineO/ValueSet.h>
#include <engine/COMMONIN.H>
#include <engine/Tables.h>
#include <engine/COMMONIN.H>
#include <engine/Engine.h>


#define RTRACE TRACE

bool CEngineDriver::LoadApplDics()
{
    for( DICT* pDicT : m_engineData->dictionaries_pre80 )
    {
        if( pDicT->GetDataDict() != nullptr )
        {
            if( !m_pEngineArea->LoadOneDic(pDicT) )
                return false;
        }
    }

    return true;
}

#define COMMON_SECT       0

bool CEngineArea::LoadOneDic(DICT* pDicT)
{
    bool bDictLoadedOK = true;
    const CDataDict* pDataDict = pDicT->GetDataDict();

    io_Dic = WS2CS(pDicT->GetName());
    io_Var.Empty();
    io_Err = 0;

    // passing definition of level-ids to pDicT
    pDicT->SetMaxLevel( (int)pDataDict->GetNumLevels() );
    int iRealMaxLevel = 0;          // TODO: WHAT ABOUT 1 Level, no qlen???
    int iLevelsIdLen = 0;
    for( int i = 0; i < (int)MaxNumberLevels; i++ ) {
        pDicT->qloc[i] = 0;
        pDicT->qlen[i] = 0;

        if( i < pDicT->GetMaxLevel() ) {
            const DictLevel& dict_level = pDataDict->GetLevel(i);
            const CDictRecord* pIdRec = dict_level.GetIdItemsRec();

            for( int j = 0; j < pIdRec->GetNumItems(); ++j )
            {
                const CDictItem* pIdItem = pIdRec->GetItem(j);

                if( j == 0 )
                    pDicT->qloc[i] = pIdItem->GetStart();

                pDicT->qlen[i] += pIdItem->GetLen();
            }

            if( pDicT->qlen[i] > 0 ) {
                iRealMaxLevel++;
                iLevelsIdLen += pDicT->qlen[i];
            }
        }
    }
    pDicT->SetLevelsIdLen( iLevelsIdLen );
    //  if( iRealMaxLevel != pDicT->GetMaxLevel() ) {
    //      WHAT???
    //  }

    // passing definition of record-id to pDicT
    pDicT->sloc = pDataDict->GetRecTypeStart();
    pDicT->slen = pDataDict->GetRecTypeLen();
    if( pDicT->slen > MAX_RECTYPECODE ) {
        CString csMsg;
        csMsg.Format(_T("Record Type Length too large (%d). Max is %d"), pDicT->slen, MAX_RECTYPECODE );
        issaerror( MessageType::Abort, MGF::OpenMessage, csMsg.GetString() );
    }

    pDicT->SYMTfsec = -1;
    pDicT->SYMTlsec = -1;

    // LOADING SECTIONS (required by Groups loading)
    for( int iTotRecs = 0, iLevel = 0; !io_Err && iLevel < pDicT->GetMaxLevel(); iLevel++ ) {
        const DictLevel& dict_level = pDataDict->GetLevel(iLevel);
        const CDictRecord* pRecord = dict_level.GetIdItemsRec();

        // section-level is 0 for Working' dicts        // victor Oct 25, 00
        int     iSecLevel = ( pDicT->GetSubType() != SymbolSubType::Work ) ? iLevel + 1 : 0;

        dictloadsection( pDicT, pRecord, iSecLevel, COMMON_SECT );      // was 'iLevel + 1'

        int     iNumRecords = dict_level.GetNumRecords();

        for( int iRecNum = 0; !io_Err && iRecNum < iNumRecords; iRecNum++ ) {
            pRecord = dict_level.GetRecord(iRecNum);
            dictloadsection( pDicT, pRecord, iSecLevel, iTotRecs + 1 ); // was 'iLevel + 1'
            iTotRecs++;
        }
    }

    // though secondary indexes were removed in CSPro 7.0, .pen files may still have references to them
    // so we insert an unused symbol here to account for the previously existing index
    m_engineData->AddSymbol(std::make_unique<DeprecatedSymbol>(_T("_MAIN_IDX"), SymbolType::Index_Unused));

    bDictLoadedOK = m_pEngineDriver->LoadApplMessage(); // evaluate & prepare message

    if( bDictLoadedOK ) {
        pDicT->Common_Start();
    }

    return bDictLoadedOK;
}

//------------------------------------------------------------------------
//      dictloadsection      drives section loading
//------------------------------------------------------------------------
void CEngineArea::dictloadsection(DICT* pDicT, const CDictRecord* pRecord, int iLevel, int iRecNum)
{
    io_Var.Empty();

    // inserting section symbol
    auto pSecT = std::make_shared<SECT>(CS2WS(pRecord->GetName()), m_pEngineDriver);
    int iSymSec = m_engineData->AddSymbol(pSecT);

    for( const CString& alias : pRecord->GetAliases() )
        GetSymbolTable().AddAlias(alias, *pSecT);

    const_cast<CDictRecord*>(pRecord)->SetSymbol(iSymSec);

    // link Sec to Dic
    pSecT->SetDictRecord( pRecord );
    pSecT->SetDicT( pDicT );                            // victor Sep 05, 00
    pSecT->SYMTowner = pDicT->GetSymbolIndex();

    ChainSymbol(pDicT->SYMTlsec, pSecT.get());

    pSecT->SetCommon( iRecNum == COMMON_SECT );         // RHF Jul 04, 2000
    pSecT->SetSubType( pDicT->GetSubType() );           // victor Jan 04, 00


    // setup Sect' basic data
    pSecT->SetLevel( iLevel );
    // COMPLETE ( is_digit( srp->sectlevel ) ) ? srp->sectlevel - '0' : -1;

    const TCHAR* pSecCode = pRecord->GetRecTypeVal();
    int     iCodeLen = _tcslen(pSecCode);

    for( int iCode = 0; iCode < MAX_RECTYPECODE; iCode++ ) {
        pSecT->code[iCode] = ( iCode < iCodeLen ) ? pSecCode[iCode] : _T(' ');
    }

    if( iRecNum == COMMON_SECT ) {
        _tmemset( pSecT->code, _T(' '), MAX_RECTYPECODE );

        // if pDicT->slen == 0, what happens to a dict with 1 multiple record?

        if( pDicT->slen > 0 ) {
            const csprochar*   codes = _T("");
            if( iLevel >= 1 && iLevel <= 4 )
                pSecT->code[0] = codes[iLevel - 1];
        }
    }

    // COMPLETE pSecT->SetSpecialSection( ( srp->special == 1 ) );
    pSecT->SetSpecialSection( false ); // Flag for compctab add work Variables

    pSecT->SetMinOccs( pRecord->GetRequired() ? 1 : 0 );

    int maxrecs = pRecord->GetMaxRecs();
    if( maxrecs > MAX_MAX_RECS ) {
        CString csMsg;
        csMsg.Format(_T("Too many occurrences (%d) for section '%s'. Max is %d."), maxrecs, pRecord->GetName().GetString(), MAX_MAX_RECS );
        issaerror( MessageType::Abort, MGF::OpenMessage, csMsg.GetString() );
    }
    pSecT->SetMaxOccs( maxrecs );

    // spronrecs                        // COMPLETE not used in ISSA

    // loading Vars
    int     iNumItems = pRecord->GetNumItems();
    int     iSymMainItem = 0;           // SYMT of last true item

    for( int iItem = 0; !io_Err && iItem < iNumItems; iItem++ )
    {
        const CDictItem* pItem = pRecord->GetItem(iItem);
        const int iSymItem = dictloadvariable(pSecT.get(), pItem, iSymMainItem);

        // update SYMT of last true item (for any subsequent subitem)
        if( pItem->GetItemType() != ItemType::Subitem )
            iSymMainItem = iSymItem;
    }
}


//------------------------------------------------------------------------
//      dictloadvariable      loads a variable
//------------------------------------------------------------------------
int CEngineArea::dictloadvariable(SECT* pSecT, const CDictItem* pItem, int iSymMainItem)
{
    int iSymSec = pSecT->GetSymbolIndex();

    // inserting Var' symbol
    io_Var = pItem->GetName();

    auto pVarT = std::make_shared<VART>(CS2WS(pItem->GetName()), m_pEngineDriver);

    // link variable to Section and Item
    pVarT->SetDictItem(pItem);
    pVarT->SYMTowner = iSymSec;
    pVarT->SetSPT(pSecT);

    int iSymVar;
    Symbol* symbol_for_aliases;

    if( IsBinary(*pItem) )
    {
        // BINARY_TYPES_TO_ENGINE_TODO ... for now, binary items are added as both a VART and an EngineItem,
        // but only the EngineItem is added to the symbol table
        iSymVar = m_engineData->AddSymbol(pVarT, Logic::SymbolTable::NameMapAddition::DoNotAdd);

        auto engine_item = std::make_shared<EngineItem>(*pVarT);
        m_engineData->AddSymbol(engine_item);

        symbol_for_aliases = engine_item.get();
    }

    else
    {
        iSymVar = m_engineData->AddSymbol(pVarT);
        symbol_for_aliases = pVarT.get();
    }

    for( const CString& alias : pItem->GetAliases() )
        GetSymbolTable().AddAlias(alias, *symbol_for_aliases);

    const_cast<CDictItem*>(pItem)->SetSymbol(iSymVar);

    // set logical & physical features
    pVarT->SetSubType(pSecT->GetSubType());
    pVarT->SetLocation(pItem->GetStart());
    pVarT->SetLength(pItem->GetLen());

    // see FlowLoad.cpp for associated Form & field features

    // owner Item (for sub-items only)                  // victor Aug 07, 99
    if( pItem->GetItemType() == ItemType::Subitem )
        pVarT->SetSubItemOf(iSymMainItem);

    // setup variable' basic data
    const ContentType content_type = pItem->GetContentType();

    if( content_type == ContentType::Numeric )
    {
        pVarT->SetFmt('N');
    }

    else if( content_type == ContentType::Alpha )
    {
        pVarT->SetFmt('X');
    }

    else if( IsBinary(content_type) )
    {
        pVarT->SetFmt('N'); // BINARY_TYPES_TO_ENGINE_TODO temporarily adding binary dictionary items as numerics
    }

    else // ALPHAUPPER
    {
        CONTENT_TYPE_REFACTOR::LOOK_AT("remove ALPHAUPPER and process other types properly");
        pVarT->SetFmt('A');
    }

    pVarT->SetDecimals(pItem->GetDecimal());
    pVarT->SetZeroFill(pItem->GetZeroFill());
    pVarT->SetDecChar(pItem->GetDecChar());

    // dimensions management:
    // (a) set both MaxOccs and Class                   // victor jul 04, 00
    pVarT->SetMaxOccs( pItem->GetOccurs() );            // victor jul 04, 00

    // (b) remaining dimension' features
    if( !pVarT->SetDimFeatures() )      // was 'SetVarIsArray'
    {
        io_Err = 91;                    // SEE PROVISIONAL CONSTRAINTS
        return 0;
    }

    // installs into its owner Section
    pSecT->AddVariable(pVarT.get());

    // load value sets
    bool first_value_set = true;

    for( const DictValueSet& dict_value_set : pItem->GetValueSets() )
    {
        auto value_set = std::make_shared<ValueSet>(dict_value_set, pVarT.get(), *m_engineData);
        m_engineData->AddSymbol(value_set);

        for( const CString& alias : dict_value_set.GetAliases() )
            GetSymbolTable().AddAlias(alias, *value_set);

        const_cast<DictValueSet&>(dict_value_set).SetSymbolIndex(value_set->GetSymbolIndex());

        if( first_value_set )
        {
            pVarT->SetBaseValueSet(value_set);
            first_value_set = false;
        }
    }

    return iSymVar;
}
