#pragma once

//---------------------------------------------------------------------------
//  File name: FlowCore.h
//
//  Description:
//          Header for the core of flow control
//
//  History:    Date       Author   Comment
//              ---------------------------
//              08 Jan 01   vc      Created for final expansion to 3 dimensions
//              26 Jan 01   vc      Adding NullOcc (as HeadOcc & TailOcc) to single, scalar groups
//              08 May 01   vc      Adding aIndex to Item' atoms
//              30 May 01   vc      Adding support for level' processing
//              07 Jun 01   vc      New methods getting atom' information on its Level
//              14 Jun 01   vc      Add methods getting atom' information on its Groups & Item
//              29 Sep 04   rcl     Adds toString() method to ease the debugging process.
//

//---------------------------------------------------------------------------

#include <Zissalib/CsKernel.h>
#include <Zissalib/flowatom.h>
#include <engine/dimens.h>
#include <zDictO/DDClass.h>

class CSymbolFlow;
class CEngineDriver;
class CEngineArea;
struct EngineData;


class CFlowBefore : public CFlowAtom {
public:
    CFlowBefore()  { SetAtomType( CFlowAtom::AtomType::BeforeStrip ); }
};

//////////////////////////////////////////////////////////////////////////
// Generic definitions for objects inside Flow
//
// rcl, Jun 14, 2004

// CFlowIndexFwd: object with forward information
class CFlowIndexFwd : public CFlowAtom
{
protected:
    C3DObject m_theObject;

public:
    CFlowIndexFwd() {}
    CFlowIndexFwd( int iSymbol, const CNDIndexes& theIndex ) : m_theObject( iSymbol, theIndex ) {}
#ifdef _DEBUG
    virtual std::wstring toString();
#endif
};

// CFlowIndexFwdBwd: object with forward *and* backward information
class CFlowIndexFwdBwd : public CFlowIndexFwd
{
protected:
    C3DIndexes m_IndexBwd;

public:
    CFlowIndexFwdBwd() : CFlowIndexFwd(), m_IndexBwd(ONE_BASED, DEFAULT_INDEXES ) {}
    CFlowIndexFwdBwd( int iSymbol, const CNDIndexes& theIndexFwd, const CNDIndexes& theIndexBwd ) :
        CFlowIndexFwd( iSymbol, theIndexFwd ),
        m_IndexBwd( theIndexBwd )
    {
    }
#ifdef _DEBUG
    virtual std::wstring toString();
#endif
};

//////////////////////////////////////////////////////////////////////////

// CFlowDouble: templated class derived from CFlowIndexFwdBwd
//              this is the one you use to create specialized types (back and forward)
template <CFlowAtom::AtomType T>
class CFlowDouble : public CFlowIndexFwdBwd
{
public:
    CFlowDouble( int iSymGroup, const CNDIndexes& theIndexFwd, const CNDIndexes& theIndexBwd ) :
      CFlowIndexFwdBwd( iSymGroup, theIndexFwd, theIndexBwd )
      {
          SetAtomType( T );
      }
      void        SetSymbol( int iSymbol ) { m_theObject.SetSymbol( iSymbol ); }
      int         GetSymbol() const        { return m_theObject.GetSymbol(); }
      C3DIndexes& GetIndexFwd()            { return m_theObject.getIndexes(); }
      C3DIndexes& GetIndexBwd()            { return m_IndexBwd; }
};

typedef CFlowDouble<CFlowAtom::AtomType::GroupHead> CFlowGroupHead;
typedef CFlowDouble<CFlowAtom::AtomType::HTOcc> CFlowHTOccBase;

class CFlowHTOcc : public CFlowHTOccBase
{
    OccType     m_xOccType;
public:
    CFlowHTOcc( int iSymGroup, const CNDIndexes& theIndexFwd, const CNDIndexes& theIndexBwd, OccType xOccType )
        : CFlowHTOccBase( iSymGroup, theIndexFwd, theIndexBwd )
    {
        m_xOccType = xOccType;
    }
    OccType     GetOccType() const { return m_xOccType; }
};

//////////////////////////////////////////////////////////////////////////

// CFlowSingle: templated class derived from CFlowIndexFwd
//              this is the one you use to create specialized types (only forward)
template <CFlowAtom::AtomType T>
class CFlowSingle : public CFlowIndexFwd
{
public:
    CFlowSingle( int iSymGroup, const CNDIndexes& theIndex )
        : CFlowIndexFwd( iSymGroup, theIndex )
    {
        SetAtomType( T );
    }
public:
    void        SetSymbol( int iSymbol ) { m_theObject.SetSymbol(iSymbol);  }
    int         GetSymbol() const        { return m_theObject.GetSymbol();  }
    C3DIndexes& GetIndex()               { return m_theObject.getIndexes(); }
};

typedef CFlowSingle<CFlowAtom::AtomType::GroupTail> CFlowGroupTail;
typedef CFlowSingle<CFlowAtom::AtomType::Item> CFlowItem;
typedef CFlowSingle<CFlowAtom::AtomType::BlockHead> CFlowBlockHead;
typedef CFlowSingle<CFlowAtom::AtomType::BlockTail> CFlowBlockTail;


class CFlowBeyond : public CFlowAtom {
public:
    CFlowBeyond() { SetAtomType( AtomType::BeyondStrip ); }
};

//---------------------------------------------------------------------------
//
//  class CFlowCore : public CObject
//
//
//  Description:
//      Support basic flow actions
//
//  * Basic model:
//
//      A flow is used to generate an extended flow-strip, atom by atom.  Clients
//      have only elementary actions to move accross the flow-strip
//
//          FlowStripNext
//          FlowStripPrev
//          FlowStripRestart
//
//      An additional method, FlowStripCurr, returns the current atom (there always is one)
//
//      The flow-strip is composed of atoms having the type of object (Group, Item),
//      its nature (head, tail) and a full occurrence-descriptor:
//
//          GroupHead
//            HTOccur
//              Item
//            HTOccur
//          GroupTail
//
//      A HTOccur plays both the roles of Head AND Tail of an occurrence.  Each
//      occurrence is always preceded by one HTOccur and it is always followed by
//      another HTOccur.  Each HTOccur has the info of which the indexes are for
//      both the "forward" and the "backwards" occurrence, i.e. for a 3-occs
//      subitem-index there will be
//
//            HTOccur Forward(r,i,1) Backwards(r,i,1)
//            HTOccur Forward(r,i,2) Backwards(r,i,1)
//            HTOccur Forward(r,i,3) Backwards(r,i,2)
//            HTOccur Forward(r,i,3) Backwards(r,i,3)
//
//  * Utilization:
//
//      Clients of this core support should interpret every answers for their
//      own purposes.  For instance, the executor-driver could use the answer
//      provided by FlowStripNext as follows:
//
//      ... answer "GroupHead":
//          - to prepare the static bitmap,
//          - to prepare the dynamic bitmap,
//          - to get the "actual" bitmap by "anding" both static and dynamic bitmaps,
//          - to execute the associated procs indicated by the "actual" bitmap
//
//
//  Methods:
//
//  Construction/destruction/initialization
//      CFlowCore                   Constructor
//      ~CFlowCore                  Destructor
//
//  Attached FLOW
//      SetSymbolFlow               Set the FLOW owning this flow-strip (and this into the FLOW)
//
//  Moving accross the flow-strip
//      FlowStripCurr               The current atom
//      FlowStripNext               The atom following the current (don't move if none)
//      FlowStripPrev               The atom preceding the current (don't move if none)
//      FlowStripRestart            Reset the current atom before the begining of the flow-strip
//      FlowStripGetCurrIndex       Return the current atom' index (warning - exclusive for CsDriver)
//      FlowStripSetCurrIndex       Reset the current atom to a given atom (warning - exclusive for CsDriver)
//      GetLowestFieldAtom          Returns the atom of the lowest field in the flow
//      SetLowestFieldAtom          Set the atom of the lowest field in the flow
//
//  Flow-strip management
//      FlowStripGetSize            Return the size of the flow-strip
//      FlowStripBuild              Drives the generation of the flow-strip
//      FlowStripBuildGroupMembers  Generates one atom for each member (group/item) of a group
//      FlowStripBuildOneGroup      Generates the fragment of flow-strip for one group
//      FlowStripBuildNullOcc       Generates one atom for a null occurrence (for single-occurrence groups)
//      FlowStripBuildHeadOcc       Generates one atom for a head of one occurrence
//      FlowStripBuildTailOcc       Generates one atom for a tail of one occurrence
//      FlowStripAddBefore          Generates one before-strip atom
//      FlowStripAddGroupHead       Generates one atom for the head of one group
//      FlowStripAddGroupTail       Generates one atom for the tail of one group
//      FlowStripAddHTOcc           Generates one atom for a head or a tail of one occurrence
//      FlowStripAddItem            Generates one atom for an item (now with full indexes, victor May 08, 01)
//      FlowStripAddBeyond          Generates one beyond-strip atom
//
//  Support for level' processing                       // victor May 30, 01
//      GetMinLevel                 Return the minimum level in the strip
//      GetMaxLevel                 Return the maximum level in the strip
//      GetNumLevels                Return the actual number of levels in the strip
//      GetLevelHeadIndex           Return the GroupHead-atom' index for a given level
//      GetLevelTailIndex           Return the GroupTail-atom' index for a given level
//      SetLevelHeadIndex           Set a given GroupHead-atom' index for a given level
//      SetLevelTailIndex           Set a given GroupTail-atom' index for a given level
//
//  Getting atom' information on its Level, Group & Item// victor Jun 07, 01
//      ... for a given atom (or the current-atom by default)
//      IsLevelHead                 True if the atom is a Level' GroupHead
//      IsLevelTail                 True if the atom is a Level' GroupTail
//      IsLevelHeadTail             True if the atom is a Level' GroupHead/GroupTail, depending on a given parameter
//      GetAtomLevel                Return the Level for the atom
//      GetAtomLevelSymbol          Return the symbol of the Level-group for the atom
//      GetAtomLevelInfo            Return either the Level or the symbol of the Level-group for the atom, depending on a given parameter
//      GetAtomGroupSymbol          Return the Group' symbol of the atom
//      GetAtomItemSymbol           Return the Item' symbol of the atom (0 if not an item-atom)
//      GetAtomGroupInfo            Return the Group' symbol of the atom and, optionally, places the Item' symbol of the atom into a given integer (0 if not an item-atom)
//
//  Link to engine
//      SetEngineDriver             Installs engine environment
//
//  Operators
//      <none>
//
//---------------------------------------------------------------------------

class CFlowCore : public CObject
{
// --- Data members --------------------------------------------------------

    // --- attached FLOW
private:
    CSymbolFlow*    m_pSymbolFlow;

    // --- cursor (current atom)
private:
    int             m_iCurAtom;
    CFlowAtom*      m_pLowestFieldAtom;

    // --- flow-strip
private:
    std::vector<CFlowAtom*> m_aAtomStrip;

    // --- support for level' processing                // victor May 30, 01
private:
    int             m_iMinLevel;        // minimum level at this strip
    int             m_iMaxLevel;        // maximum level at this strip
    int             m_aLevelHeadIndex[MaxNumberLevels + 1];
    int             m_aLevelTailIndex[MaxNumberLevels + 1];

    // --- engine links
public:
    CEngineDriver*  m_pEngineDriver;
    CEngineArea*    m_pEngineArea;
    EngineData*     m_engineData;

// --- Methods -------------------------------------------------------------
    // --- construction/destruction
public:
    CFlowCore();
    ~CFlowCore();

    // --- attached FLOW
    void            SetSymbolFlow( CSymbolFlow* pFlow );

    // --- moving accross the flow-strip
public:
    CFlowAtom*      FlowStripCurr()                           { return m_aAtomStrip[m_iCurAtom]; }
    CFlowAtom*      FlowStripNext();                         
    CFlowAtom*      FlowStripPrev();                         
    void            FlowStripRestart()                        { FlowStripSetCurrIndex( 0 ); } // point to the BeforeStrip' atom
    int             FlowStripGetCurrIndex()                   { return m_iCurAtom; }  // victor May 30, 01
    void            FlowStripSetCurrIndex( int iAtom )        { m_iCurAtom = iAtom; } // victor May 30, 01
    CFlowAtom*      GetLowestFieldAtom()                      { return m_pLowestFieldAtom; }
private:
    void            SetLowestFieldAtom( CFlowAtom* pAtom )    { m_pLowestFieldAtom = pAtom; }

    // --- flow-strip management
public:
    int             FlowStripGetSize()                        { return (int)m_aAtomStrip.size(); }
    int             FlowStripBuild();
private:
    void            FlowStripBuildGroupMembers( int iSymGroup );
    void            FlowStripBuildOneGroup( int iSymGroup );
    void            FlowStripBuildNullOcc( int iSymGroup, CFlowAtom::OccType xOccType );
    void            FlowStripBuildHeadOcc( int iSymGroup, CFlowAtom::OccType xOccType );
    void            FlowStripBuildTailOcc( int iSymGroup );
    void            FlowStripAddBefore();
    void            FlowStripAddGroupHead( int iSymGroup, int* aIndexFwd, int* aIndexBwd );
    void            FlowStripAddGroupTail( int iSymGroup, int* aIndex );
    void            FlowStripAddHTOcc( int iSymGroup, int* aIndexFwd, int* aIndexBwd, CFlowAtom::OccType xOccType );
    void            FlowStripAddItem( int iSymItem, int* aIndex );
    void            FlowStripAddBlock(int block_symbol_index, int* aIndex, int* group_iterator);
    void            FlowStripAddBlockHead(int block_symbol_index, int* aIndex);
    void            FlowStripAddBlockTail(int block_symbol_index, int* aIndex);
    void            FlowStripAddBeyond();

    // --- support for level' processing                // victor May 30, 01
public:
    int             GetMinLevel() const                      { return m_iMinLevel; }
    int             GetMaxLevel() const                      { return m_iMaxLevel; }
    int             GetNumLevels() const                     { return ( m_iMinLevel < 0 || m_iMaxLevel < 0 ) ? 0 : ( 1 + m_iMaxLevel - m_iMinLevel ); }
    int             GetLevelHeadIndex( int iLevel );
    int             GetLevelTailIndex( int iLevel );
private:
    bool            SetLevelHeadIndex( int iLevel, int iAtom );
    bool            SetLevelTailIndex( int iLevel, int iAtom );

    // --- getting atom' information on its Level       // victor Jun 07, 01
public:
    bool            IsLevelHead( CFlowAtom* pAtom = NULL )    { return IsLevelHeadTail( pAtom, true ); }
    bool            IsLevelTail( CFlowAtom* pAtom = NULL )    { return IsLevelHeadTail( pAtom, false ); }
private:
    bool            IsLevelHeadTail( CFlowAtom* pAtom, bool bAskForHead );
public:
    int             GetAtomLevel( CFlowAtom* pAtom = NULL )   { return GetAtomLevelInfo( pAtom, true ); }
    int             GetAtomLevelSymbol( CFlowAtom* pAtom = NULL ){ return GetAtomLevelInfo( pAtom, false ); }
private:
    int             GetAtomLevelInfo( CFlowAtom* pAtom, bool bAskForLevel );
public:
    int             GetAtomGroupSymbol( CFlowAtom* pAtom )    { return GetAtomGroupInfo( pAtom ); }
    int             GetAtomItemSymbol( CFlowAtom* pAtom );
    int             GetAtomGroupInfo( CFlowAtom* pAtom, int* pSymVar = NULL );

    // --- engine links
public:
    void            SetEngineDriver( CEngineDriver* pEngineDriver );

private:
    const Logic::SymbolTable& GetSymbolTable() const;
};
