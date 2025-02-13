#pragma once

//---------------------------------------------------------------------------
//  File name: GroupT.h
//
//  Description:
//          Header for engine-GroupT class
//
//  History:    Date       Author   Comment
//              ---------------------------
//              15 Jul 99   vc      Created
//              16 Dec 99   vc      Adding flow management
//              23 Mar 00   vc      Adding DataOccs
//              10 Jun 00   vc      Adding ptrs (to improve execution speed)
//              09 Aug 00   RHF     Add insert/delete/sort group occurrence
//              16 Aug 00   RHF     Add occurrences tree manager
//              31 Oct 00   RHF     Add opening var
//              15 Jan 01   vc      Link to procedures for 3-D implementation
//              10 May 01   RHF     Add 'Trip' method
//              28 May 01   vc      Move 'AdjustTotalOccs' method from EngineArea to GROUPT class
//              30 Apr 04   rcl     3d Occurrences handling
//
//---------------------------------------------------------------------------
#include <zDictO/DDClass.h>
#include <Zissalib/CsKernel.h>
#include <engine/3dException.h>
#include <engine/ChainedSymbol.h>
#include <zEngineO/AllSymbolDeclarations.h>
#include <zEngineO/RunnableSymbol.h>

class CEngineDriver;
class CEngineArea;
class CDEGroup;
class CDEFormBase;
struct EngineData;
class GroupVisitor;

#pragma warning(disable:4786)
#pragma warning(disable:4290) // complete exception specification not being considered
#include <Zissalib/OccurrenceInfoSet.h>

//---------------------------------------------------------------------------
//
//  class CSymbolGroup : public ChainedSymbol
//  (best known as class GROUPT)
//
//
//  Methods:
//
//  Construction/destruction/initialization
//      CSymbolGroup                Constructor
//      ~CSymbolgroup               Destructor
//      Init                        Initialization
//
//  Basic data
//      GetFlow                     Return the FLOW* owning this Group
//      GetLevel                    Return the Group' Level
//      GetSource                   Return the source of the Group (FrmFile or DcfFile, see 'enum eGroupSource' below)
//      GetGroupType                Return the type of the Group (Level/GroupOrRecord/MultItem, see 'enum eGroupType' below)
//      SetFlow                     Install a given FLOW* as the Flow owning this Group
//      SetLevel                    Set the Group' Level
//      SetSource                   Set the source of the Group
//      SetGroupType                Set the type of the Group
//      GetDepthInFlow              Return the depth-in-flow of the Group
//      SetDepthInFlow              Calculates and set the depth-in-flow of the Group
//
//  Linked IMSA Group
//      GetCDEGroup                 Return the CDEGroup* associated to the Group
//      SetCDEGroup                 Set a given CDEGroup* as the object associated to the Group
//      DeleteCDEGroup              Destroy the CDEGroup associated to the Group (for Groups where a dummy CDEGroup was specially created)
//
//  Owner-and-parent GroupT' accelerators
//      GetOwnerGPT                 Return the GROUPT* of the owner-Group
//      GetParentGPT                Return the GROUPT* of the parent-Group
//      SetParentGPT                Set a given GROUPT* as the parent-Group
//      SetOwnerGPT                 Set a given GROUPT* as the owner-Group
//
//  Group dimension
//      SetDimAndParentGPT
//      SetDimAndParentGPT          Search and set the GroupT' pointer of the "effective parent"
//                                  GroupT for occurrences-control purposes, and the number of
//                                  dimensions to get this GroupT fully qualified for compiler
//                                  and executor purposes
//      GetNumDim                   Return the number of dimensions of the Group
//      GetDimType                  Return the intrinsic dimension-type of the Group
//      SetDimType                  Set a given dimension-type as the intrinsic dimension-type of the Group
//
//    Occurrences control
//      GetMinOccs                  Return the structural min-occs of the Group
//      GetMaxOccs                  Return the structural min-occs of the Group
//      SetMinOccs                  Set a given number as the structural min-occs of the Group
//      SetMaxOccs                  Set a given number as the structural max-occs of the Group
//      GetCurrentOccurrences       Return the current-occurrence of the Group
//      GetTotalOccurrences         Return the total-occurrences of the Group
//      GetDataOccurrences          Return the data-occurrences of the Group
//      SetCurrentOccurrences       Set a given number as the current-occurrence of the Group
//      SetTotalOccurrences         Set a given number as the total-occurrences of the Group
//      SetDataOccurrences          Set a given number as the data-occurrences of the Group
//      AdjustTotalOccs             Synchronizes, according to PathOn/PathOff, the total-occurrences with the current-occurrence
//      GetFirstExOccurrence        Set and return the first execution-occurrence for automatic loops on the Group
//      GetNextExOccurrence         Set and return the next execution-occurrence for automatic loops on the Group
//      GetLastExOccurrence         Set and return the last execution-occurrence for automatic loops on the Group
//      GetPrevExOccurrence         Set and return the previous execution-occurrence for automatic loops on the Group                        // victor Jun 10, 00
//      GetCurrentExOccurrence      Return the current execution-occurrence for automatic loops on the Group
//
//  Records management
//      GetRecord                   Returns the SECT* belonging to a given record-number
//      AddRecord                   Add a SECT* to the records-array
//      FindRecord                  Locate and return the record-number for a given SECT*
//      CreateRecordArray           Create the records-array
//      CalculateOccurrence         Calculate the number of occurrence of the Group (adjusting related Groups if needed)
//
//  Bound conditions and relationships
//      IsAncestor                  RHF Oct 31, 2000
//      SearchSonOfAncestor         RHF Dec 20, 2000
//
//  List of items
//      GetNumItems                 Returns the number of items in the item-list
//      GetItemIndex                Returns the index in the item-list (and its flow-order) for a given symbol-number
//      IsAnItem                    Returns true if a given index is in the item-list
//      IsLastItem                  Returns true if the item at a given index in the item-list is the last item
//      GetItemSymbol               Return the symbol-number of the item at a given intex in the item-list
//      GetItemLevel                Return the Level of the item at a given intex in the item-list
//      GetFlowOrder                Return the flow-order of the item at a given intex in the item-list
//      DecreaseNumItems            Decrease the number of items in the item-list
//      IncreaseNumItems            Increase the number of items in the item-list
//      SetItemDescription          Set the item-info into a given slot of the item-list
//      SetNumItems                 Set the number of items in the item-list
//      CreateItemList              Create the item-list for a given number of items
//      DestroyItemList             Erases the item-list
//
//  Occurrences' tree management                        // RHF Aug 16, 2000
//      OccTreeFree                 RHF Aug 16, 2000
//
//  Multipurpose flag                                   // RHF Dec 20, 2000
//      SetFlag                     Set the value of the multi-purpose flag to a given number
//      GetFlag                     Return the value of the multi-purpose flag
//
//  General support
//      GetRootGPT                  Return the root-GROUPT*
//      GetLevelGPT                 Return the GROUPT* for a given Level
//      GetLevelSymbol              Return the symbol-number of the Group at a given Level
//      GetSymbol                   Return the symbol-number of the Group
//      GetOwnerSymbol              Return the symbol-number of the owner-group
//      GetLabel                    Provides a text description of the Group' label into a given text area
//
//  Flow support
//      SearchNextItemInGroup       Locates the next item in the Group
//      InitOneGroup                Initialize the occurrences of the Group
//
//  Deleting and inserting occurrences                  // RHF Aug 10, 2000
//      InsertOcc                   RHF Aug 10, 2000
//      DeleteOcc                   RHF Aug 10, 2000
//      SortOcc                     RHF Aug 10, 2000
//      DoMoveOcc                   RHF Aug 10, 2000
//      Trip                        RHF May 10, 2001
//
//---------------------------------------------------------------------------


typedef struct {
    int     SYMTitem;
    int     iFlowOrder;                 // order in global flow
    int     index;                      // index inside owner GROUPT (0 ... m_iNumItems - 1)
} GRITEM;


//
class CLoopControl;
class CLoopInstruction;


class CSymbolGroup : public ChainedSymbol, public RunnableSymbol
{
public:
    // GroupSource: GroupT entries can be created either from the FRM file
    //              or from the DCF file
    //
    enum class Source { Uninitialized = 0, FrmFile = 1, DcfFile = 2 };
    //
    // GroupType  : the type can be
    //            - Level: a level-group
    //            - GroupOrRecord: it is either a group coming from the FRM file,
    //              or a group built over the remaining items of a record of DCF
    //            - MultItem: a group created to encapsulate a Mult item, in order
    //              to allow for a normalized processing of occurrences of the
    //              encapsulated Mult item (both for FRM and DDW sources)
    //
    enum    eGroupType { Level = 1, GroupOrRecord = 2, MultItem = 3 };

    enum    eGroupOperation { GrouptDoNothing, GrouptInsertOcc, GrouptDeleteOcc, GrouptSortOcc, GrouptInsertOccAfter };

// --- Data members --------------------------------------------------------
private:
    // --- basic data
    FLOW*   m_pFlow;                    // FLOW owning this GroupT
    Source  m_GroupSource;
    int     m_GroupType;                // 1: Level, 2: Group/Record, 3: Mult.item
    int     m_iLevel;                   // Group Level (0: process, 1-4: visible hierarchy)
    int     m_iDepthInFlow;             // depth-in-flow (1 for level' groups) // victor Jan 16, 01
public:
    int     SYMTfirst;                  // first item
    int     SYMTlast;                   // last  item

    // --- linked IMSA Group (NULL for Level 0)
private:
    CDEFormBase* m_pCDEGroupOrLevel;    // CDEGroup or CDELevel // RHF Mar 17, 2000
    bool         m_bCanDelete;

    // --- owner-and-parent GroupT' accelerators        // victor Jun 10, 00
private:
    GROUPT* m_pOwnerGroupT;             // pointer to formal parent
    GROUPT* m_pParentGroupT;            // pointer to effective parent

    // --- group dimension
private:
    int     m_iNumDim;                  // # of dimensions of this GroupT
    CDimension::VDimType m_eDimType;    // dimension type

    // --- occurrences control
private:
    int     m_iMinOccs;                 // Physical occurrences: minimum
    int     m_iMaxOccs;                 //                       maximum
    int     m_iCurOccurrence;           // Current occurrence of this Group
    int     m_iTotalOccs;               // Maximum occurrence reached in this Group
    int     m_iDataOccs;                // Maximum occurrence in source data (always >= m_iTotalOccs) // victor Mar 23, 00
    int     m_iExOccur;                 // Engine-managed occ   // victor Jun 10, 00
    int     m_iHighOccs;                // Highest number occurrences of this Group  // BMD 14 Aug 2003
    int     m_iPartialOccs;             // Partial number occurrences of this Group. Use when a case is partially saved // RHF May 07, 2004

    std::vector<bool> m_occVisibility;      // GHM 20141015 for the showocc and hideocc functions
    std::map<int, LabelSet> m_mapSavedOccLabels;

    // --- records management
private:
    std::vector<SECT*> m_aRecords;     // RHF Mar 01, 2001

    // --- list of items
private:
    int     m_iNumItems;
    GRITEM* m_aItemList;

    // --- multipurpose flag
private:
    int     m_iFlag;                    // multipurpose flag // RHF Dec 20, 2000

    // --- other information
    int     m_iAbsoluteFlowOrder;

    // --- engine links
public:
    CEngineArea*    m_pEngineArea;
    CEngineDriver*  m_pEngineDriver;                    // RHF Aug 09, 2000
    EngineData*     m_engineData;


// --- Methods -------------------------------------------------------------
    // --- construction/destruction/initialization
    // RHF INIC Aug 16, 2000
public:
    CSymbolGroup(std::wstring name, CEngineDriver* pEngineDriver);
    ~CSymbolGroup()
    {
        // TRACE( "LEAK: Destroying occ tree for group %s\n", (LPCTSTR)GetName() );
        OccTreeFree();
    }
    // RHF END Aug 16, 2000

    // --- basic data
public:
    FLOW*   GetFlow( void )                        { return m_pFlow; }
    const FLOW* GetFlow() const                    { return m_pFlow; }
    int     GetLevel( void ) const                 { return m_iLevel; }
    Source  GetSource() const                      { return m_GroupSource; }
    int     GetGroupType( void ) const             { return m_GroupType; }
    void    SetFlow( FLOW* pOwnerFlow )            { m_pFlow = pOwnerFlow; }
    void    SetLevel( int iLevel )                 { m_iLevel = iLevel; }
    void    SetSource(Source source)               { m_GroupSource = source; }
    void    SetGroupType( int iType )              { m_GroupType = iType; }
    int     GetDepthInFlow( void )                 { return m_iDepthInFlow; } // victor Jan 16, 01
    void    SetDepthInFlow( void );                                           // victor Jan 16, 01
    int     GetExOccur()                           { return m_iExOccur; }
    void    SetExOccur( int iExOccur )             { m_iExOccur = iExOccur; }

    // --- linked IMSA Group
public:
    CDEGroup* GetCDEGroup() const                   { return (CDEGroup*)m_pCDEGroupOrLevel; }
    void    SetCDEGroup( CDEFormBase* pCDEGroupOrLevel ) { m_pCDEGroupOrLevel = pCDEGroupOrLevel; }
    void    DeleteCDEGroup( void );

    // --- owner-and-parent GroupT' accelerators        // victor Jun 10, 00
public:
    const GROUPT* GetOwnerGPT() const              { return m_pOwnerGroupT; }
    GROUPT* GetOwnerGPT()                          { return m_pOwnerGroupT; }
    const GROUPT* GetParentGPT() const             { return m_pParentGroupT; }
    GROUPT* GetParentGPT()                         { return m_pParentGroupT; }
    void    SetParentGPT( GROUPT* pParentGroupT )  { m_pParentGroupT = pParentGroupT; }// RHF Jul 31, 2000
    void    SetOwnerGPT( void );

    // --- group dimension
public:
    void    SetDimAndParentGPT( void );
    int     GetNumDim( void ) const                { return m_iNumDim; }
    CDimension::VDimType GetDimType( void ) const  { return m_eDimType; }

    void SetDimType( CDimension::VDimType xType );

private:
    //int  GetDataOccurrences( int aInfo[DIM_MAXDIM] ) const;
    //void SetDataOccurrences( int aInfo[DIM_MAXDIM], int iOcc );

public:
    int GetCurrentOccurrences( int aInfo[DIM_MAXDIM] ) const;
    void SetCurrentOccurrences( int aInfo[DIM_MAXDIM], int iOcc );

    int  GetTotalOccurrences( const CNDIndexes& theIndex ) const;       // rcl, Jun 24, 2004
    void SetTotalOccurrences( const CNDIndexes& theIndex, int iOcc );   // rcl, Jun 24, 2004
    int  GetDataOccurrences(  const CNDIndexes& theIndex ) const;       // rcl, Sept 9, 2004
    void SetDataOccurrences(  const CNDIndexes& theIndex, int iOcc );   // rcl, Sept 9, 2004
    int  GetCurrentOccurrences( const CNDIndexes& theIndex ) const;     // rcl, Sept 9, 2004
    void SetCurrentOccurrences( const CNDIndexes& theIndex, int iOcc ); // rcl, Sept 9, 2004

private:
    // Use any of these constants for iWhichOcc
    // const int CALCULATE_CURRENTOCC = 1;
    // const int CALCULATE_TOTALOCC   = 2;
    // const int CALCULATE_DATAOCC    = 3;
    // (these constants are defined in Groupt.cpp)
    void CalculateAndSetOccurrences( int iOcc, int iWhichOcc );
public:
    void CalculateAndSetCurrentOccurrences( int iOcc );
    void CalculateAndSetDataOccurrences( int iOcc );
    void CalculateAndSetTotalOccurrences( int iOcc );

private:
    // CalcCurrent3DObject_internal is called from
    //    CalcCurrent3DObject &
    //    CalcCurrent3DObjectEx methods.
    void CalcCurrent3DObject_internal(bool bUseEx) /*throw(C3DException)*/;
    C3DObject m_curr3dObject;
public:
    void CalcCurrent3DObject() /*throw(C3DException)*/;
    void CalcCurrent3DObjectEx() /*throw(C3DException)*/;
    C3DObject GetCurrent3DObject();

    // --- occurrences' tree
private:
    OccurrenceInfoSet* m_pOccTree;

    //void SetDimType( CDimension::VDimType xType )  { m_eDimType = xType; }

    // --- occurrences control
public:
    int     GetMinOccs( void ) const               { return m_iMinOccs; }
    int     GetMaxOccs( void ) const               { return m_iMaxOccs; }
    void    SetMinOccs( int iMinOccs )             { m_iMinOccs = iMinOccs; }
    void    SetMaxOccs( int iMaxOccs )             { m_iMaxOccs = iMaxOccs; }

    int     GetCurrentOccurrences() const { return m_iCurOccurrence; }
    int     GetTotalOccurrences() const   { return m_iTotalOccs; };
    int     GetDataOccurrences() const    { return m_iDataOccs; }

    void    SetCurrentOccurrences( int iOcc );
    void    SetTotalOccurrences( int iOcc );
    void    SetDataOccurrences( int iOcc );             // victor Mar 23, 00

    void    AdjustTotalOccs( void );                    // victor May 28, 01
    void    AdjustTotalOccs( CNDIndexes& theIndex );    // rcl, Nov 02, 04
private:
    int     GetMaxExOcc( void );
public:
    int     GetFirstExOccurrence( void );               // victor Jun 10, 00
    int     GetNextExOccurrence( void );                // victor Jun 10, 00
    int     GetLastExOccurrence( void );                // victor Jun 10, 00
    int     GetPrevExOccurrence( void );                // victor Jun 10, 00
    int     GetCurrentExOccurrence( void ) const;       // RHF Aug 07, 2000
    void    SetCurrentExOccurrence( int iExOccur ); // RHF Jul 09, 2001
    int     GetHighOccurrences( void )             { return m_iHighOccs; }  // BMD 14 Aug 2003
    void    SetHighOccurrences( int iOccs )        { m_iHighOccs = iOccs; } // BMD 14 Aug 2003

         // RHF INIC May 07, 2004
    int     GetPartialOccurrences( void )               { return m_iPartialOccs; }
    void    SetPartialOccurrences( int iOccs )          { m_iPartialOccs = iOccs; }
    // RHF END May 07, 2004

    bool    IsOccVisible(int iOcc) const                { return ( iOcc < (int)m_occVisibility.size() ) ? m_occVisibility[iOcc] : true; }
    void    SetOccVisibility(int iOcc, bool visible);
    void    ResetOccVisibilities()                      { m_occVisibility.clear(); }

    void    SaveOccLabel(int iOcc);
    void    ResetOccLabels();

    // --- records management
    // RHF INIC Mar 01, 2001
    SECT*   GetRecord( int iRecNum ) const {
        if( iRecNum >= (int)m_aRecords.size() )
            return NULL;
        return m_aRecords[iRecNum];
    }

    int     FindRecord( SECT* pSecT ) const {
        for( size_t i = 0; i < m_aRecords.size(); ++i )
            if( pSecT == m_aRecords[i] )
                return (int)i;
        return -1;
    }

    bool RecordsAreIdentical(const GROUPT& rhs_group) const;

private:

    void    AddRecord( SECT* pSecT )               { m_aRecords.emplace_back(pSecT); }

public:
    int     CalculateOccurrence( void );
    void    CreateRecordArray( void );
    // RHF END Mar 01, 2001

    // --- bound conditions and relationships
public:
    bool    IsAncestor( int iSymGroup, bool bCountingOccurrences, bool bStartFromThis=false ); // RHF Oct 31, 2000

    // --- link to procedures (old fashion)
//  <none>

    // --- list of items
public:
    int     GetNumItems( void ) const              { return m_iNumItems; }
    int     GetItemIndex( int iSymItem, int* pFlowOrder = NULL ) const;
    bool    IsAnItem( int iItem ) const            { return( iItem >= 0 && iItem < GetNumItems() ); }
    bool    IsLastItem( int iItem ) const          { return( iItem + 1 >= GetNumItems() ); }
    int     GetItemSymbol( int iItem ) const       { return( IsAnItem( iItem ) ? m_aItemList[iItem].SYMTitem : 0 ); }
    const CDictItem* GetFirstDictItem() const;
    Symbol* GetItemSymbolPtr( int iItem ) const;

    int     GetItemLevel( int iItem );
    int     GetFlowOrder( int iItem ) const        { return( IsAnItem( iItem ) ? m_aItemList[iItem].iFlowOrder : 0 ); }
    void    DecreaseNumItems( void )               { m_iNumItems--; }
    void    IncreaseNumItems( void )               { m_iNumItems++; }
    bool    SetItemDescription( int iItem, int iSymItem, int iFlowOrder = -1 );
    void    SetNumItems( int iNumItems ) {
                bool    bCreated = CreateItemList( iNumItems ); // RHF Feb 02, 2000
                ASSERT( bCreated );
            }
    bool    CreateItemList( int iNumItems );
    void    DestroyItemList( void );

    // --- occurrences' tree management
    // RHF INIC Aug 16, 2000
public:
    void    OccTreeFree( void );
    // RHF END Aug 16, 2000

    // --- multipurpose flag
    // RHF INIC Dec 20, 2000
public:
    void    SetFlag( int iFlag )                   { m_iFlag = iFlag; }
    int     GetFlag( void )                        { return m_iFlag; }
    // RHF END Dec 20, 2000

    // --- general support
public:
    CSymbolGroup*   GetRootGPT( void );
    CSymbolGroup*   GetLevelGPT( int iLevel );
    int             GetLevelSymbol( int iLevel );
    int     GetSymbol( void ) const         { return GetSymbolIndex(); }
    int     GetOwnerSymbol( void ) const    { return SYMTowner; }
    CString GetLabel() const;

    // --- flow support
public:
    int     SearchNextItemInGroup( int iOccur, int iItem, int* pNewOccur, int* pNewItem ) const;
    void    InitOneGroup( void );

    // --- deleting and inserting occurrences
    // RHF INIC Aug 10, 2000
public:
    bool    InsertOcc( const CNDIndexes& theIndex, const CLoopControl* pLoopControl );
    bool    DeleteOcc( const CNDIndexes& theIndex, const CLoopControl* pLoopControl );
    bool    SortOcc( const CNDIndexes& theIndex, const CLoopControl* pLoopControl, int iSymVar, bool bAscending );
    bool    SortOcc( const CNDIndexes& theIndex, const CLoopControl* pLoopControl, const std::vector<int>& sort_symbols, std::vector<int>* validIndices );

//private: (DoMoveOcc changed to public on 20100106 GHM)

    bool    DoMoveOcc( const CNDIndexes& theIndex, const CLoopControl* pLoopControl, const CLoopInstruction* pLoopInstruction );
    // RHF END Aug 10, 2000
public:
    int     Trip( Symbol* sp, int iInfo, void* pInfo );// RHF May 10, 2001

    // RHF INIC Dec 10, 2003
public:
    void SetAbsoluteFlowOrder(int iAbsoluteFlowOrder) { m_iAbsoluteFlowOrder = iAbsoluteFlowOrder; }
    int GetAbsoluteFlowOrder() const                  { return m_iAbsoluteFlowOrder; }
    // RHF END Dec 10, 2003

    void SetCanDelete( bool bCanDelete ) { m_bCanDelete = bCanDelete; }
    bool CanDelete() const               { return m_bCanDelete; }

    void serialize_subclass(Serializer& ar) override;

#if defined(USE_BINARY) || defined(GENERATE_BINARY)
    virtual void accept( GroupVisitor* visitor );
#endif

private:
    const Logic::SymbolTable& GetSymbolTable() const;

private:
    int m_containerIndex = 0; // the container table index

public:
    int GetContainerIndex() const  			    { return m_containerIndex; }
    void SetContainerIndex(int container_index) { m_containerIndex = container_index; }
};
