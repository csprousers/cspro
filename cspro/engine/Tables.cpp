//--------------------------------------------------------------------------
//    TABLES.CPP     functions for Symbol Table handling
//--------------------------------------------------------------------------

#include "StandardSystemIncludes.h"
#include "Tables.h"
#ifndef  GENCODE
#define  GENCODE
#endif
#include "COMPILAD.H"
#include "Engine.h"
#include "Tables.h"
#include "Ctab.h"
#include <zEngineO/AllSymbols.h>
#include <zEngineO/EngineDictionaryFactory.h>
#include <zToolsO/Tools.h>
#include <zUtilO/MemoryHelpers.h>
#include <zDictO/DDClass.h>


//---------------------------------------------------------
//  inittables: allocates internal tables
//---------------------------------------------------------
int CEngineArea::inittables()
{
    mem_model();

    // install symbol table
    GetSymbolTable().Clear();

    CtNodenext = 0;
    if (CtNodemxent > 0) {
        CtNodebase = (int*) calloc( CtNodemxent, sizeof(int) );

        if (CtNodebase == NULL)
            issaerror(MessageType::Abort, 1000, _T("CTAB"));
    }

    // marks level-id list as empty
    for (size_t i = 0; i < MaxNumberLevels; i++)
        QidVars[i][0] = -1;

    return TRUE;
}


//----------------------------------------------------------
//  mem_model : setup size of tables to be used
//----------------------------------------------------------
void CEngineArea::mem_model()
{
    int iEstimatedTables = 200; //99;
    //Savy increasing memory to fix Tom's problem when tabulation application crashes.
    //the current model allocates 192K . Increasing it to ~19MB by increasing the multiplying factor from 10 t0 1000
    CtNodemxent = iEstimatedTables * 1000 * (sizeof(CTNODE) + 2 * sizeof(CTRANGE));
}


int CEngineArea::ownerdic(int iSymbol) const
{
    int iSymDic = 0;

    if( iSymbol > 0 )
    {
        switch( NPT(iSymbol)->GetType() )
        {
            case SymbolType::Variable:
            {
                int iSymSec = VPT(iSymbol)->GetOwnerSec();
                if( iSymSec > 0 )
                    iSymDic = SPT(iSymSec)->SYMTowner;
                break;
            }

            case SymbolType::Section:
                iSymDic = SPT(iSymbol)->SYMTowner;
                break;

            case SymbolType::Pre80Dictionary:
                iSymDic = iSymbol; // OK - is the Dic itself
                break;
        }
    }

    ASSERT(iSymDic > 0);

    return iSymDic;
}


//----------------------------------------------------------
//  ChainSymbol   : chain symbol to the previous one
//----------------------------------------------------------
void CEngineArea::ChainSymbol(int previous_chained_symbol_index, ChainedSymbol* chained_symbol) const
{
    ChainedSymbol* previous_chained_symbol = nullptr;
    const Symbol* chained_symbol_owner_symbol = NPT(chained_symbol->SYMTowner);
    int* owner_first_symbol_index = nullptr;
    int* owner_last_symbol_index = nullptr;

    if( previous_chained_symbol_index >= 0 )
    {
        previous_chained_symbol = (ChainedSymbol*)NPT(previous_chained_symbol_index);
        ASSERT(previous_chained_symbol->IsOneOf(SymbolType::Section, SymbolType::Group, SymbolType::Variable));
    }

    if( chained_symbol_owner_symbol->IsA(SymbolType::Section) )
    {
        if( chained_symbol->IsA(SymbolType::Variable) )
        {
            owner_first_symbol_index = &((SECT*)chained_symbol_owner_symbol)->SYMTfvar;
            owner_last_symbol_index = &((SECT*)chained_symbol_owner_symbol)->SYMTlvar;
        }
    }

    else if( chained_symbol_owner_symbol->IsA(SymbolType::Pre80Dictionary) )
    {
        owner_first_symbol_index = &((DICT*)chained_symbol_owner_symbol)->SYMTfsec;
        owner_last_symbol_index = &((DICT*)chained_symbol_owner_symbol)->SYMTlsec;
    }

    int next_chained_symbol_index = ( previous_chained_symbol != nullptr ) ? previous_chained_symbol->SYMTfwd : *owner_first_symbol_index;

    chained_symbol->SYMTfwd = next_chained_symbol_index;

    if( previous_chained_symbol != nullptr )
    {
        previous_chained_symbol->SYMTfwd = chained_symbol->GetSymbolIndex();
        previous_chained_symbol->next_symbol = chained_symbol;
    }


    else if( owner_first_symbol_index != nullptr )
        *owner_first_symbol_index = chained_symbol->GetSymbolIndex();

    if( next_chained_symbol_index < 0 && owner_last_symbol_index != nullptr )
        *owner_last_symbol_index = chained_symbol->GetSymbolIndex();
}


std::unique_ptr<Symbol> CEngineArea::CreateSymbol(std::wstring symbol_name, SymbolType symbol_type, SymbolSubType symbol_subtype)
{
    auto create_symbol = [&]() -> std::unique_ptr<Symbol>
    {
        switch( symbol_type )
        {
            case SymbolType::Dictionary:
            case SymbolType::Record:
                return EngineDictionaryFactory::CreateSymbolOnDeserialization(std::move(symbol_name), symbol_type, *m_engineData);

            case SymbolType::Array:
                return std::make_unique<LogicArray>(std::move(symbol_name));

            case SymbolType::Audio:
                return std::make_unique<LogicAudio>(std::move(symbol_name));

            case SymbolType::Document:
                return std::make_unique<LogicDocument>(std::move(symbol_name));

            case SymbolType::File:
                return std::make_unique<LogicFile>(std::move(symbol_name));

            case SymbolType::Geometry:
                return std::make_unique<LogicGeometry>(std::move(symbol_name));

            case SymbolType::HashMap:
                return std::make_unique<LogicHashMap>(std::move(symbol_name));

            case SymbolType::Image:
                return std::make_unique<LogicImage>(std::move(symbol_name));

            case SymbolType::List:
                return ( symbol_subtype == SymbolSubType::ValueSetListWrapper ) ? std::make_unique<ValueSetListWrapper>(std::move(symbol_name), *m_engineData) :
                                                                                  std::make_unique<LogicList>(std::move(symbol_name));

            case SymbolType::Map:
                return std::make_unique<LogicMap>(std::move(symbol_name));

            case SymbolType::NamedFrequency:
                return std::make_unique<NamedFrequency>(std::move(symbol_name));

            case SymbolType::Pff:
                return std::make_unique<LogicPff>(std::move(symbol_name));

            case SymbolType::Relation:
                return std::make_unique<RELT>(std::move(symbol_name), GetSymbolTable());

            case SymbolType::Report:
                return std::make_unique<Report>(std::move(symbol_name), std::wstring());

            case SymbolType::Section:
                return std::make_unique<SECT>(std::move(symbol_name), m_pEngineDriver);

            case SymbolType::SystemApp:
                return std::make_unique<SystemApp>(std::move(symbol_name));

            case SymbolType::UserFunction:
                return std::make_unique<UserFunction>(std::move(symbol_name), *m_engineData);

            case SymbolType::Variable:
                return std::make_unique<VART>(std::move(symbol_name), m_pEngineDriver);

            case SymbolType::ValueSet:
                return std::make_unique<DynamicValueSet>(std::move(symbol_name), *m_engineData);

            case SymbolType::WorkString:
                return ( symbol_subtype == SymbolSubType::WorkAlpha ) ? std::make_unique<WorkAlpha>(std::move(symbol_name)) :
                                                                        std::make_unique<WorkString>(std::move(symbol_name));

            case SymbolType::WorkVariable:
                return std::make_unique<WorkVariable>(std::move(symbol_name));

            default:
                return ReturnProgrammingError(nullptr);
        }
    };

    std::unique_ptr<Symbol> symbol = create_symbol();
    symbol->SetSubType(symbol_subtype);
    return symbol;
}


int CEngineArea::SymbolTableSearch(const StringNoCase& full_symbol_name, SymbolType preferred_symbol_type,
                                   const std::vector<SymbolType>* allowable_symbol_types) const
{
    // search for a symbol, allowing for dot notation
    try
    {
        const Symbol& symbol = GetSymbolTable().FindSymbolWithDotNotation(full_symbol_name, preferred_symbol_type, allowable_symbol_types);
        return symbol.GetSymbolIndex();
    }

    catch(...)
    {
        return 0;
    }
}


std::vector<Symbol*> CEngineArea::SymbolTableSearchAllSymbols(const StringNoCase& full_symbol_name) const
{
    // if using dot notation, use the general search function
    if( full_symbol_name.find('.') != StringNoCase::npos )
    {
        int symbol_index = SymbolTableSearch(full_symbol_name, SymbolType::None, nullptr);

        if( symbol_index > 0 )
            return { NPT(symbol_index) };

        return { };
    }

    // otherwise return the full set of symbols
    else
    {
        return GetSymbolTable().FindSymbols(full_symbol_name);
    }

}


CEngineArea::CEngineArea()
    :   m_engineData(std::make_shared<EngineData>(CreateEngineAccessor(this))),
        m_Appl(std::make_shared<APPL>())
{
    Init();
}


void CEngineArea::SetEngineDriver(CEngineDriver* pEngineDriver)
{
    m_pEngineDriver = pEngineDriver;
    m_pEngineArea = this;
    m_pEngineDefines = pEngineDriver->m_pEngineDefines;
    m_pEngineSettings = pEngineDriver->m_pEngineSettings;
    m_engineData->application = pEngineDriver->m_pApplication;
}


void CEngineArea::Init()
{
    // --- engine links
    m_pEngineDriver = NULL;
    m_pEngineArea = NULL;
    m_pEngineDefines = NULL;
    m_pEngineSettings = NULL;

    // --- symbols management
    m_bMarkUnusedSymbol = false; // RHF 19/11/99

    // --- main tables
    m_CtNodebase = NULL;
    m_CtNodemxent = 0;
    m_CtNodenext = 0;

    memset(m_Breakvars, 0, sizeof(int) * MAXBREAKVARS);
    memset(m_Breaklvar, 0, sizeof(TCHAR) * MAXBREAKVARS);
    m_Breaklevel = 0;
    m_Breaknvars = 0;
    m_aCtabBreakId.clear(); // RHF Apr 16, 2003
    m_CtabBreakHighLevel = 0; // RHF Apr 16, 2003

#ifndef USE_BINARY
    // --- export management
    m_pCurExport = NULL;// current export
    ResetExpoSeqNo(); // seq # for assigning    // victor Dec 18, 00
#endif // !USE_BINARY
}


CEngineArea::~CEngineArea()
{
    FreeTables();

    Init();
}


bool CEngineArea::IsLevel(int iSymbol, bool bMustBeInPrimaryFlow) {
    // IsLevel: check 'iSymbol' is a Level-group and,   // victor May 10, 00
    //      ... if 'bMustBeInPrimaryFlow' is true, check it belongs to the primary flow
    //      --- added to support every checking formerly tied to DI objects
    // ... was in CEngineCompFunc
    bool bOK = false;
    bool bIsGroup = (m_pEngineArea->IsSymbolTypeGR(iSymbol));

    if (bIsGroup) {
        GROUPT* pGroupT = GPT(iSymbol);
        bool bIsLevelGroup = (pGroupT->GetGroupType() == 1);

        if (bIsLevelGroup)
            bOK = (!bMustBeInPrimaryFlow
                    || pGroupT->GetFlow() == m_pEngineDriver->GetPrimaryFlow());
    }

    return bOK;
}


int CEngineArea::GetGroupOfSymbol(int symbol_index) const
{
    const Symbol& symbol = NPT_Ref(symbol_index);

    int group_symbol_index =
        symbol.IsA(SymbolType::Group)     ? symbol_index :
        symbol.IsA(SymbolType::Block)     ? assert_cast<const EngineBlock&>(symbol).GetGroupT()->GetSymbolIndex() :
        symbol.IsA(SymbolType::Variable)  ? assert_cast<const VART&>(symbol).GetOwnerGroup() :
                                            ReturnProgrammingError(0);
    ASSERT(group_symbol_index > 0);

    return group_symbol_index;
}


GROUPT* CEngineArea::GetGroupTOfSymbol(int symbol_index) const
{
    int group_symbol_index = GetGroupOfSymbol(symbol_index);
    return ( group_symbol_index > 0 ) ? GPT(group_symbol_index) : nullptr;
}


int CEngineArea::GetSectionOfSymbol(int symbol_index) const
{
    const Symbol* symbol = NPT(symbol_index);
    int section_symbol_index = 0;

    if( symbol->IsA(SymbolType::Section) )
    {
        section_symbol_index = symbol_index;
    }

    else
    {
        const VART* variable = symbol->IsA(SymbolType::Variable) ? assert_cast<const VART*>(symbol) :
                               symbol->IsA(SymbolType::ValueSet) ? assert_cast<const ValueSet*>(symbol)->GetVarT() :
                                                                   nullptr;

        if( variable != nullptr )
            section_symbol_index = variable->GetOwnerSec();
    }

    ASSERT(section_symbol_index > 0);

    return section_symbol_index;
}
