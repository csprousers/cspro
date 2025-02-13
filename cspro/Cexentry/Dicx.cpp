// Dicx functions
#include "STDAFX.H"
#include <engine/Dicx.h>
#include <engine/Engine.h>
#include <engine/RELATION.H>
#include <zEngineO/EngineCaseConstructionReporter.h>
#include <zToolsO/Tools.h>
#include <zCaseO/Case.h>
#include <zDataO/CaseIterator.h>
#include <zDataO/DataRepository.h>
#include <zDataO/NullRepositoryIterators.h>
#include <ZBRIDGEO/npff.h>


//////////////////////////////////////////////////////////////////////////////
//
// --- Initialization
//
//////////////////////////////////////////////////////////////////////////////

void DICX::Init()
{
    // --- related objects
    m_pDicT          = NULL;            // DICT* brother

    // --- basic data
    m_iMaxRecLength  = 0;               // max record size among Dict' sections
    m_iKeyHolderLen  = 0;               // key-holders' length

    // --- related items in the Dictionary
    pRelations       = NULL;                            // RHF 12/8/99

    // --- data management
    m_pRecArea       = NULL;            // record area for I/O of one record // formerly 'recad'
    m_pKeyHolderArea = NULL;            // key-holders' area (for 3 keys)
    current_key      = NULL;            // key-holder for current case
    last_key         = NULL;            // key-holder for last case

    LastOpenLevel    = -1;              // last Level opened    // RHF 22/9/98
    level            = 0;               // current node level
    nextlevel        = 0;               // next node key level
    lastlevel        = 0;               // last node key level

    // legacy data - perhaps obsolete, check it         // victor Jul 10, 00
    entermode        = false;           // 1 => in data entry  0 => other (external dict)

    m_rd = nullptr;
}

//////////////////////////////////////////////////////////////////////////////
//
// --- CEngineArea::methods
//
//////////////////////////////////////////////////////////////////////////////

int CEngineArea::marksymbolunused( Symbol* sp ) // RHF 19/11/99
{
    int     iSymbol = sp->GetSymbolIndex();

    if( is_digit( NPT(iSymbol)->GetName().front() ) )
        return 0;

    if( NPT(iSymbol)->IsA(SymbolType::Variable) ) {
        VART*   pVarT = VPT(iSymbol);

// RHF COM Aug 21, 2000if( pVarT->IsUsed() ) {
        if( !pVarT->IsUsed() ) { // RHF Aug 21, 2000
            const CDictItem* pItem = pVarT->GetDictItem();
            int iItemSym = pItem->GetSymbol();

            if( m_bMarkUnusedSymbol ) {
                ASSERT( iItemSym > 0 );
                const_cast<CDictItem*>(pItem)->SetSymbol( iItemSym * -1 ); // set as negative ... DD_STD_REFACTOR_TODO why?
            }
            else {
                ASSERT( iItemSym < 0 );
                const_cast<CDictItem*>(pItem)->SetSymbol( iItemSym * -1 ); // set as positive ... DD_STD_REFACTOR_TODO why?
            }
        }
    }

    return 1;
}


int CEngineArea::DicxStart()
{
    for( DICT* pDicT : m_engineData->dictionaries_pre80 )
    {
        DICX* pDicX = pDicT->GetDicX();
        const CDataDict* pDataDict = pDicT->GetDataDict();

        if( pDataDict == NULL )
            continue;

        // TRUCO: Set unused item with negative symbol in order to be ignored in
        // Generate
        m_bMarkUnusedSymbol = true;   //RHF 19/11/99
        dicttrip( pDicT, (pDictTripFunc) &CEngineArea::marksymbolunused );// RHF 19/11/99

        CRelations* pRelations = new CRelations( *pDataDict );
        pRelations->Generate();

        // TRUCO: Set unused item with positive symbol
        m_bMarkUnusedSymbol = false; // RHF 19/11/99
        dicttrip( pDicT, (pDictTripFunc) &CEngineArea::marksymbolunused ); // RHF 19/11/99

        // Assign to variables the respective related-table slot index
        int iSlotNum = 0;
        for( const auto& idt : pRelations->GetItemsRelations() ) {
            for( const auto& pItemBase : idt->GetRelated() ) {
                if( pItemBase->GetSymbolIdx() >= 1 ) {
                    VPX(pItemBase->GetSymbolIdx())->iRelatedSlot = iSlotNum;
                }
            }
            ++iSlotNum;
        }

        pDicX->SetEngineDriver( m_pEngineDriver );
        pDicX->pRelations = pRelations;
    }

    return TRUE;
}

void CEngineArea::DicxEnd()
{
    for( DICT* pDicT : m_engineData->dictionaries_pre80 )
    {
        DICX* pDicX = pDicT->GetDicX();
        delete pDicX->pRelations;
        pDicX->pRelations = nullptr;
        pDicX->StopRuntime();
    }
}


void DICX::StartRuntime()
{
    DICT* pDicT = GetDicT();

    if( pDicT->GetDataDict() == nullptr )
        return;

    m_rd = new RuntimeData;
    m_rd->m_caseIterationMethod = CaseIterationMethod::KeyOrder;
    m_rd->m_caseIterationOrder = CaseIterationOrder::Ascending;
    m_rd->m_caseIterationCaseStatus = CaseIterationCaseStatus::NotDeletedOnly;

    // initialize the case access
    CaseAccess* case_access = pDicT->GetCaseAccess();

    // full access is required of entry inputs, batch inputs when there is an output,
	// special outputs, and writeable external dictionaries
    bool set_requires_full_access = false;
    bool batch_input_mode = false;

    switch( pDicT->GetSubType() )
    {
        case SymbolSubType::Input:
        {
            if( pDicT->m_pEngineArea->m_Appl->ApplicationType == ModuleType::Entry )
            {
                set_requires_full_access = true;
            }

            else
            {
                batch_input_mode = true;

                if( pDicT->m_pEngineDriver->m_pPifFile->UsingOutputData() )
                {
                    set_requires_full_access = true;
                }

                // if running an entry application in Run As Batch mode, all dictionary items need to be read in
                else if( pDicT->m_pEngineDriver->m_pEngineSettings->HasCheckRanges() || pDicT->m_pEngineDriver->m_pEngineSettings->HasSkipStruc() )
                {
                    case_access->SetUseAllDictionaryItems();
                }
            }

            break;
        }

        case SymbolSubType::Output:
        {
            set_requires_full_access = true;
            break;
        }

        case SymbolSubType::External:
        {
            set_requires_full_access = pDicT->GetWriteable();
            break;
        }

        default:
        {
            ASSERT(pDicT->GetSubType() == SymbolSubType::Work);
        }
    }

    if( set_requires_full_access )
        case_access->SetRequiresFullAccess();

    case_access->Initialize();

    // create the case construction reporter, only using the process summary for the batch
    // input file so that record counts for external dictionaries don't get added in
    std::optional<std::function<void(const Case&)>> update_case_callback;

#ifdef WIN_DESKTOP
    if( batch_input_mode )
        update_case_callback = [this](const Case& data_case) { m_pEngineDriver->GetLister()->SetMessageSource(data_case); };
#endif

    case_access->SetCaseConstructionReporter(std::make_shared<EngineCaseConstructionReporter>(
        m_pEngineDriver->GetSharedSystemMessageIssuer(),
        batch_input_mode ? m_pEngineDriver->GetProcessSummary() : nullptr,
        std::move(update_case_callback)));

    // create the case
    m_rd->m_case = case_access->CreateCase(true);
}


void DICX::StopRuntime()
{
    CloseDataRepository();

    delete m_rd;
    m_rd = nullptr;
}


void DICX::SetDataRepository(std::shared_ptr<DataRepository> data_repository)
{
    ASSERT(data_repository != nullptr);

    CloseDataRepository();

    *current_key = 0; // reset the read keys
    *last_key = 0;

    m_dataRepository = std::move(data_repository);
}


void DICX::CloseDataRepository()
{
    if( m_rd == nullptr )
        return;

    ClearLastSearchedKey();
    StopCaseIterator();

    // CR_TODO refactor below
    try
    {
        if( m_dataRepository != nullptr )
        {
            m_rd->m_lastClosedConnectionString = m_dataRepository->GetConnectionString();
            m_dataRepository->Close();
        }
    }

    catch( const DataRepositoryException::Error& exception )
    {
        issaerror(MessageType::Error, 10105, exception.GetErrorMessage().c_str());
    }

    m_dataRepository.reset();
}


std::tuple<CaseIterationMethod, CaseIterationOrder, CaseIterationCaseStatus> DICX::GetDictionaryAccessParameters(int dictionary_access) const
{
    // the first bit of a byte indicates if the value is set; bytes by order: method || order || status
    return std::make_tuple(
        ( ( dictionary_access & 0x800000 ) != 0 ) ? (CaseIterationMethod)( ( dictionary_access >> 16 ) & 0x7F ) : m_rd->m_caseIterationMethod,
        ( ( dictionary_access & 0x8000 ) != 0 ) ? (CaseIterationOrder)( ( dictionary_access >> 8 ) & 0x7F ) : m_rd->m_caseIterationOrder,
        ( ( dictionary_access & 0x80 ) != 0 ) ? (CaseIterationCaseStatus)( dictionary_access & 0x7F ) : m_rd->m_caseIterationCaseStatus
        );
}


void DICX::CreateCaseIterator(CaseIteratorStyle case_iterator_style, std::optional<CaseKey> starting_key/* = std::nullopt*/,
                              int dictionary_access/* = 0*/, std::optional<CString> key_prefix/* =std::nullopt*/,
                              CaseIterationContent iteration_content/* = CaseIterationContent::Case*/)
{
    StopCaseIterator();

    // if the case key isn't defined, the iterator will start from the beginning of the file
    CaseIterationStartType iteration_start_type = CaseIterationStartType::GreaterThan;
    const CaseKey* case_key = nullptr;

    if( case_iterator_style == CaseIteratorStyle::FromCurrentPosition )
    {
        // if a case has been loaded, create an iterator from it
        if( _tcslen(current_key) != 0 )
            case_key = m_rd->m_case.get();
    }

    else if( case_iterator_style == CaseIteratorStyle::FromLastSearchedCaseKey )
    {
        ASSERT(IsLastSearchedCaseKeyDefined());
        iteration_start_type = CaseIterationStartType::GreaterThanEquals;
        case_key = &(*m_rd->m_lastSearchedCaseKey);
    }
    
    else if( case_iterator_style == CaseIteratorStyle::FromNextKey )
    {
        ASSERT(starting_key.has_value());
        case_key = &(*starting_key);
    }

    CaseIterationMethod case_iteration_method;
    CaseIterationOrder case_iteration_order;
    CaseIterationCaseStatus case_iteration_case_status;
    std::tie(case_iteration_method, case_iteration_order, case_iteration_case_status) = GetDictionaryAccessParameters(dictionary_access);

    std::unique_ptr<CaseIteratorParameters> start_parameters;
    
    if( case_key != nullptr || key_prefix.has_value() )
    {
        // flip the order for a descending iterator
        if( case_iteration_order == CaseIterationOrder::Descending )
        {
            if( iteration_start_type == CaseIterationStartType::GreaterThan )
            {
                iteration_start_type = CaseIterationStartType::LessThan;
            }

            else if( iteration_start_type == CaseIterationStartType::GreaterThanEquals )
            {
                iteration_start_type = CaseIterationStartType::LessThanEquals;
            }
        }

        if( case_iteration_method == CaseIterationMethod::KeyOrder )
        {
            start_parameters = std::make_unique<CaseIteratorParameters>(iteration_start_type, ( case_key != nullptr ) ? case_key->GetKey() : CString(), key_prefix);
        }

        else
        {
            start_parameters = std::make_unique<CaseIteratorParameters>(iteration_start_type, ( case_key != nullptr ) ? case_key->GetPositionInRepository() : -1, key_prefix);
        }
    }

    try
    {
        m_rd->m_caseIterator = m_dataRepository->CreateIterator(iteration_content, case_iteration_case_status,
                                                                case_iteration_method, case_iteration_order, start_parameters.get());
    }

    catch(...)
    {
        // set the iterator to a "null iterator" one so that m_caseIterator is not null
        m_rd->m_caseIterator = std::make_unique<NullRepositoryCaseIterator>();
        throw;
    }
}


void DICX::StopCaseIterator()
{
    try
    {
        m_rd->m_caseIterator.reset();
    }

    catch( const DataRepositoryException::Error& exception )
    {
        issaerror(MessageType::Error, 10105, exception.GetErrorMessage().c_str());
    }
}


void DICX::ResetCaseObjects()
{
    m_rd->m_case->Reset();
    m_rd->m_notesByCaseLevel.clear();
}
