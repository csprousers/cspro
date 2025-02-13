#pragma once

//---------------------------------------------------------------------------
//  File name: EngArea.h
//
//  Description:
//          Header for the EngineArea class
//
//  History:    Date       Author   Comment
//              ---------------------------
//              ?? Oct 99   RHF     creation
//              06 Mar 00   RCH     Adding value-set and value-label handling
//              23 Mar 00   vc      Adding 'DataOccurrences' to GROUPT management
//              04 Apr 01   vc      Tailoring for RepMgr compatibility
//              28 May 01   vc      Move 'AjustTotalOccs" to GROUPT class
//
//---------------------------------------------------------------------------

#include <zLogicO/Symbol.h>
#include <zLogicO/SymbolTable.h>
#include <engine/Tables.h>
#include <zEngineO/EngineData.h>
#include <zEngineO/RuntimeEvent.h>
#include <zEngineO/SymbolCalculator.h>

class Frequency;


//---------------------------------------------------------------------------
//
//  CLASS NAME: EngArea
//
//
//  PROTOTYPE:  class EngArea
//
//
//  OBJECTIVE:
//
//      ???????????????????????????????????????????????????????????????
//      ???????????????????????????????????????????????????????????????
//
//
//  METHODS:
//
//      Construction/destruction, initialization, link to EngineDriver
//          EngArea             Constructor
//          ~EngArea            Destructor
//          Init                ???
//          SetEngineDriver     ???
//
//      Main tables management
//          inittables          ???
//          tablesend           ???
//          DicxStart           ???
//          SecxStart           ???
//          VarxStart           ???
//          FreeTables          ???
//          DicxEnd             ???
//          SecxEnd             ???
//          VartEnd             ???
//
//      Related to symbl table utilisation
//          ChainSymbol         ???
//          ownerdic            Return the owner dictionary' symbol of a given symbol
//          marksymbolunused    ???
//
//      Export management
//          SetCurExport
//          GetCurExport
//          ExportAdd
//          ExportGetSize
//          ExportGetAt
//          ResetExpoSeqNo
//          GetExpoSeqNo
//          NewExpoSeqNo
//
//     Miscellaneous purpose methods?
//          mem_model           ???
//          resources           ???
//          cthighnumvalue      ???
//          setup_vmark         ???
//
//      Supporting CEngineDriver::exapplinit
//          get_acum            ???
//          get_cumarea         ???
//          LookForUsedSubItems ???
//          GetDataFileNames    ???
//
//      Labels management?
//          lab_load            ???
//          lab_readimsa        ???
//
//      'compall' support?
//          GroupTtrip          ???
//          dicttrip            ???
//
//      Application loading
//          MakeApplChildren    Inserts Appl' children (Flows/Dicts/Flow Forms) into symbol table
//          MakeApplChildrenError Issue an error message for MakeApplChildren
//          MakeApplFlowFormError Issue an error message for MakeApplChildren
//
//      Dictionary loading
//          LoadOneDic          Drives the whole loading of a dictionary
//          dictls              Drives section loading
//          dictlv              Load one variable
//
//      Related to GROUPT management
//          InitLevelGroups     Initialize all counters for a level-node and descendants in the ONLY branch instance
//          GetGroupRelationship Returns the relationship of two given groups
//          GroupMaxNumOccs     Returns the max-occs of a given group or variable
//          GroupSetCurOccurrence Set the cur-occs to a given number for a group/variable
//          GetTypeGRorVA       Returns a type (GRoup or VAriable) of a given symbol index
//          IsSymbolTypeGR      True when the symbol given is a GRoup
//          IsSymbolTypeVA      True when the symbol given is a VAriable
//
//      Operators
//          <none>
//
//---------------------------------------------------------------------------

class CEngineArea;
class CEngineDriver;
class CEngineDefines;
class CSettings;
class CDataDict;
class CDictRecord;
class CDictItem;
class CDEForm;
class CDELevel;
class CDEItemBase;
class CDEFormFile;
class CObArray;
class CEngineCompFunc;
class CExport;

typedef   int (CEngineArea::*pDictTripFunc)(void *);
typedef   int (CEngineCompFunc::*pGroupTripFunc)(Symbol*);
typedef   int (GROUPT::*pGroupTripFunc2)(Symbol*, int iInfo, void* pInfo);



/////////////////////////////////////////////////////////////////////////////
//
// CEngineArea
//
/////////////////////////////////////////////////////////////////////////////

class CEngineArea
{
// --- Data members --------------------------------------------------------

    // --- engine links
public:
    CEngineArea*            m_pEngineArea;
    CEngineDriver*          m_pEngineDriver;
    CEngineDefines*         m_pEngineDefines;
    CSettings*              m_pEngineSettings;

    // --- symbols management
private:
    bool                    m_bMarkUnusedSymbol;        // RHF 19/11/99

    // --- main tables
public:
    std::shared_ptr<EngineData> m_engineData;               // engine data
    std::shared_ptr<APPL> m_Appl;                           // Appl descriptor
                                                         
    int*                    m_CtNodebase;          //  CtNodes  table: address
    int                     m_CtNodemxent;         //                  max entries
    int                     m_CtNodenext;          //                  next free

    // the indices of local persistent variables that need to be reset
    std::set<int> m_persistentSymbolsNeedingResetSet;

#ifndef USE_BINARY
    // --- export management
private:
    CExport*                    m_pCurExport;       // current export
    std::vector<CExport*>       m_aExport;          // array of export' commands // victor Dec 18, 00
    int                         m_iExpoSeqNo;       // seq # for assigning    // victor Dec 18, 00
#endif // !USE_BINARY

    // --- break-by memory
public:
    int                     m_Breakvars[MAXBREAKVARS];
    TCHAR                   m_Breaklvar[MAXBREAKVARS];
    int                     m_Breaklevel;
    int                     m_Breaknvars;

    std::vector<CBreakById> m_aCtabBreakId;// RHF Apr 16, 2003
    int                     m_CtabBreakHighLevel;


// --- Methods -------------------------------------------------------------

    // --- construction/destruction, initialization, link to EngineDriver
public:
    CEngineArea();
    ~CEngineArea();
private:
    void    Init();
public:
    void    SetEngineDriver( CEngineDriver* pEngineDriver );

    EngineData& GetEngineData()                { return *m_engineData; }
    LogicByteCode& GetLogicByteCode()          { return m_engineData->logic_byte_code; }
    Logic::SymbolTable& GetSymbolTable() const { return m_engineData->symbol_table; }

    // --- main tables management
public:
    int     inittables();
    void    tablesend();
    int     DicxStart();
    int     SecxStart();
    int     VarxStart();
private:
    void    FreeTables();
    void    DicxEnd();
    void    SecxEnd();

    // --- symbol table utilisation
public:
    int ownerdic(int iSymbol) const;

    void ChainSymbol(int previous_chained_symbol_index, ChainedSymbol* chained_symbol) const;

    std::unique_ptr<Symbol> CreateSymbol(std::wstring symbol_name, SymbolType symbol_type, SymbolSubType symbol_subtype);

    int SymbolTableSearchWithPreference(const StringNoCase& full_symbol_name, SymbolType preferred_symbol_type) const { return SymbolTableSearch(full_symbol_name, preferred_symbol_type, nullptr); }
    int SymbolTableSearch(const StringNoCase& full_symbol_name, const std::vector<SymbolType>& allowable_symbol_types, SymbolType preferred_symbol_type = SymbolType::None) const { return SymbolTableSearch(full_symbol_name, preferred_symbol_type, &allowable_symbol_types); }
    std::vector<Symbol*> SymbolTableSearchAllSymbols(const StringNoCase& full_symbol_name) const;
private:
    int SymbolTableSearch(const StringNoCase& full_symbol_name, SymbolType preferred_symbol_type, const std::vector<SymbolType>* allowable_symbol_types) const;

private:
    static std::shared_ptr<EngineAccessor> CreateEngineAccessor(CEngineArea* pEngineArea);

private:
    int     marksymbolunused( Symbol* sp );


#ifndef USE_BINARY
    // --- export management
public:
    void    SetCurExport( CExport* pExport )    { m_pCurExport = pExport; }
    CExport* GetCurExport( void )               { return m_pCurExport; }
    void    ExportFinish( void );       // in 'tablesen.cpp' // victor Dec 18, 00
    void    ExportAdd( CExport* pExport )       { m_aExport.emplace_back( pExport ); }
    int     ExportGetSize( void ) const         { return (int)m_aExport.size(); }
    CExport* ExportGetAt( int iExpoSlot )       { return m_aExport[iExpoSlot]; }
    void    ResetExpoSeqNo( void )              { m_iExpoSeqNo = 0; }                     // victor Dec 18, 00
    int     GetExpoSeqNo( void )                { return( m_iExpoSeqNo + 1 ); }           // victor Dec 18, 00
    int     NewExpoSeqNo( void )                { return ++m_iExpoSeqNo; }                // victor Dec 18, 00
    void    ClearExports()                      { m_aExport.clear(); ResetExpoSeqNo(); }
#endif // !USE_BINARY

    // --- miscellaneous purpose methods
private:
    void    mem_model();
public:
    double  cthighnumvalue( int ct_node );
    int     setup_vmark( Symbol* ps );

    // --- supporting CEngineDriver::exapplinit
public:
    void    get_acum();
    void    get_cumarea( CTAB *ct );
    int     LookForUsedSubItems();

    // --- labels management?
public:
    std::vector<TCHAR> lab_load(int isym);

    // --- 'compall' support?
public:
    void    GroupTtrip( GROUPT* pGroupT, pGroupTripFunc func, pGroupTripFunc2 func2=NULL, int iInfo=0, void* pInfo=NULL );
    void    dicttrip( DICT *dp, pDictTripFunc func );


    // --- application loading
    // ... creating Application' children Dicts & Flows // victor Dec 27, 99
public:
    bool    MakeApplChildren();

    // --- dictionary loading                           // see 'dictload.cpp'
public:
    bool    LoadOneDic(DICT* pDicT);

private:
    void    dictloadsection(DICT* pDicT, const CDictRecord* pRecord, int iLevel, int iRecNum);
    int     dictloadvariable(SECT* pSecT, const CDictItem* pItem, int iSymMainItem);

    // --- methods related to GROUPT management
    // ... initialization for 'DeFuncs'
public:
    void    InitLevelGroups( int iLevel );

    // ... occurrences management
public:
    int     GroupMaxNumOccs( int iSymObj );
    bool    GroupSetCurOccurrence( int iSymObj, int iNewOccurrence );
    bool    GroupSetCurOccurrence( GROUPT* pGroupT, int iNewOccurrence, int iSymGroup = 0, int iSymObj = 0 ); // victor May 25, 00
    bool    GroupSetCurOccurrence( GROUPT* pGroupT, CNDIndexes& theIndex, int iSymGroup /*= 0*/, int iSymObj = 0 ); // rcl, Nov 02, 04
    bool    GroupSetCurOccurrenceUsingVar( C3DObject& theObject );

    // ... generic symbol evaluation
public:
    SymbolType GetTypeGRorVA(int iSymObj) const
    {
        if( IsSymbolTypeVA(iSymObj) )      return SymbolType::Variable;
        else if( IsSymbolTypeGR(iSymObj) ) return SymbolType::Group;
        else                               return SymbolType::Unknown;
    }

    SymbolType GetTypeGRorVAorBlock(int iSymObj) const
    {
        return ( ( iSymObj > 0 ) && NPT(iSymObj)->IsA(SymbolType::Block) ) ? SymbolType::Block : GetTypeGRorVA(iSymObj);
    }

    bool    IsLevel( int iSymbol, bool bMustBeInPrimaryFlow = false ); // was in enginecompfun

    bool    IsSymbolTypeGR( int iSymObj ) const { return( iSymObj > 0 && NPT(iSymObj)->IsA( SymbolType::Group ) ); }
    bool    IsSymbolTypeVA( int iSymObj ) const { return( iSymObj > 0 && NPT(iSymObj)->IsA( SymbolType::Variable ) ); }
    bool    IsSymbolTypeCT( int iSymObj ) const { return( iSymObj > 0 && NPT(iSymObj)->IsA( SymbolType::Crosstab ) ); }

    int GetGroupOfSymbol(int symbol_index) const;
    GROUPT* GetGroupTOfSymbol(int symbol_index) const;
    int GetSectionOfSymbol(int symbol_index) const;

#ifdef _DEBUG
    // TEST ONLY
    CString DumpGroupTName( int iSymbol );
#endif
};

    // --- aliases for public symbols
#define   Appl              (*m_pEngineArea->m_Appl)

#define   CtNodebase        m_pEngineArea->m_CtNodebase
#define   CtNodemxent       m_pEngineArea->m_CtNodemxent
#define   CtNodenext        m_pEngineArea->m_CtNodenext

#define   Breakvars         m_pEngineArea->m_Breakvars
#define   Breaklvar         m_pEngineArea->m_Breaklvar
#define   Breaklevel        m_pEngineArea->m_Breaklevel
#define   Breaknvars        m_pEngineArea->m_Breaknvars
