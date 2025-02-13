#pragma once

//---------------------------------------------------------------------------
//  File name: SecT.h
//
//  Description:
//          Header for engine-Sec class
//
//  History:    Date       Author   Comment
//              ---------------------------
//              15 Jul 99   RHF     Basic conversion
//              04 Jul 00   RHF     Adding IsCommon, etc
//              04 Jul 00   vc      Adding ptrs to related objets (improve execution speed)
//
//---------------------------------------------------------------------------

#include <engine/Defines.h>

class CEngineArea;
class CEngineDriver;
class CDictRecord;
class GroupVisitor;

#include <engine/ChainedSymbol.h>

struct SECX;                                           // victor Jul 04, 00

//---------------------------------------------------------------------------
//
//  class CSymbolSection : public ChainedSymbol
//
//  Description:
//      Engine-Section management
//
//  Construction/destruction/initialization
//      CSymbolSection              Constructor
//      ~CSymbolSection             Destructor
//      Init                        Initialize data
//
//  Related objects
//      GetSecX                     Return the pointer to the SECX entry matching this Sect
//      SetSecX                     Set the pointer to the SECX entry matching this Sect
//      GetDicT                     Return the pointer to the owner DICT of this Sect
//      SetDicT                     Set the pointer to the owner DICT of this Sec
//
//  Basic data
//      GetLevel                    Return the level of this Sect
//      SetLevel                    Set  the level of this Sect
//
//  Structural limits on occurrences
//      GetMinOccs                  Return the min-occs of this Sect
//      SetMinOccs                  Set the min-occs of this Sect
//      GetMaxOccs                  Return the max-occs of this Sect
//      SetMaxOccs                  Set the max-occs of this Sect
//
//  Other record' data
//      IsCommon                    True if the Sect is a common-Sect
//      SetCommon                   Set the common-Sect attribue to a given true/false
//      IsSpecialSection            True if the Sect is marked as "special section"
//      SetSpecialSection           Set the "special section" attribute to a given true/false
//      SetLastLocTo                Set the location (1-based) of the last character occupied in this Sect
//      GetLastLoc                  Return the location (1-based) of the last character occupied in this Sect
//      UpdateLastLoc               Recalculate the last-location
//
//  Linked IMSA Item
//      <none>
//
//  Other methods
//      AddVariable                 Add a given Var to this Sect
//
//---------------------------------------------------------------------------

class CSymbolSection : public ChainedSymbol
{

// --- Data members --------------------------------------------------------
    // --- related objects
private:
    DICT*   m_pDicT;                    // DICX* owner  // victor Jul 04, 00
    SECX*   m_pSecX;                    // SECX* brother// victor Jul 04, 00

    // --- basic data
public:
    int     SYMTfvar;                   // Symt entries with first Var  in Sect
    int     SYMTlvar;                   //                   last  Var  in Sect
    int     m_iLevel;                   // Section hierarchy Level      // formerly 'levl'
    csprochar    code[MAX_RECTYPECODE+1];    // Section identification code

    // --- structural limits on occurrences
private:
    int     m_iMinOccs;                 // Section occurrences: minimum // formerly 'smin'
    int     m_iMaxOccs;                 //                      maximum // formerly 'smax'

    // --- other record' data
private:
    bool    m_bOccGenerator;    // RHF Nov 15, 2002
    bool    m_bIsCommon;                                // RHF Jul 04, 2000
    bool    m_bSpecialSection;          // is special section for table vars  // formerly 'special'
    int     m_iLastLoc;                 // Last character occupied in section // formerly 'last'

    // --- records management
private:
    std::vector<GROUPT*> m_aGroups;     // RHF Feb 19, 2002

    // RHF INIC Nov 24, 2000
private:
    const CDictRecord* m_pDictRecord;
public:
    void SetDictRecord(const CDictRecord* pDictRecord) { m_pDictRecord = pDictRecord; }
    const CDictRecord* GetDictRecord() const           { return m_pDictRecord; }
// RHF END Nov 24, 2000


    // --- link to Engine
public:
    CEngineArea* m_pEngineArea;


// --- Methods -------------------------------------------------------------
    // --- construction/destruction/initialization
public:
    CSymbolSection(std::wstring name, CEngineDriver* pEngineDriver);

    // --- related objects
    SECX* GetSecX( void )            { return m_pSecX; }         // victor Jul 04, 00
    void SetSecX( SECX* pSecX )      { m_pSecX = pSecX; }        // victor Jul 04, 00
    DICT* GetDicT( void )            { return m_pDicT; }         // victor Jul 04, 00
    const DICT* GetDicT() const      { return m_pDicT; }
    void SetDicT( DICT* pDicT )      { m_pDicT = pDicT; }        // victor Jul 04, 00

    // --- basic data
public:
    int GetLevel( bool bOneBased = false ) const
    {
        // RHF INIC Sep 29, 2005
        if( bOneBased && m_iLevel == 0 )
            return 1;
        // RHF END Sep 29, 2005

        return m_iLevel;
    }
    void    SetLevel( int iLevel )      { m_iLevel = iLevel; }

    // --- structural limits on occurrences
public:
    int     GetMinOccs() const          { return m_iMinOccs; }
    void    SetMinOccs( int iMinOccs )  { m_iMinOccs = iMinOccs; }
    int     GetMaxOccs() const          { return m_iMaxOccs; }
    void    SetMaxOccs( int iMaxOccs )  { m_iMaxOccs = iMaxOccs; }

    // --- other record' data
public:
    bool    IsCommon() const            { return m_bIsCommon; }     // RHF Jul 04, 2000
    void    SetCommon( bool bIsCommon ) { m_bIsCommon = bIsCommon; }// RHF Jul 04, 2000
    bool    IsSpecialSection( void )    { return m_bSpecialSection; } // victor Jul 10, 00
    void    SetSpecialSection( bool bX ){ m_bSpecialSection = bX; } // victor Jul 10, 00
    void    SetLastLocTo( int iLoc = 0 ){ m_iLastLoc = iLoc; }      // victor Oct 27, 99
    int     GetLastLoc( void )          { return m_iLastLoc; }      // victor Oct 27, 99
private:
    void    UpdateLastLoc();                            // victor Oct 27, 99

    // --- linked IMSA Item
    //      <none>

    // --- other methods
public:
    void    AddVariable( VART* pVarT );

    // RHF INIC Feb 19, 2002
    // --- groupt management.. Groups involved in this Section
private:
    void    AddGroup( GROUPT* pGroupT )               { m_aGroups.emplace_back( pGroupT ); }
    int     FindGroup( GROUPT* pGroupT ) {
        for( size_t i = 0; i < m_aGroups.size(); ++i )
            if( pGroupT == m_aGroups[i] )
                return (int)i;
            return -1;
    }

public:
    GROUPT* GetGroup(int iGroupNum)             { return ( iGroupNum >= (int)m_aGroups.size() ) ? NULL : m_aGroups[iGroupNum]; }
    const GROUPT* GetGroup(int iGroupNum) const { return ( iGroupNum >= (int)m_aGroups.size() ) ? NULL : m_aGroups[iGroupNum]; }

    void    CreateGroupArray();

    void    SetOccGenerator(bool bOccGenerator) { m_bOccGenerator = bOccGenerator; }
    bool    IsOccGenerator() const              { return m_bOccGenerator; }

// RHF END Feb 19, 2002

public:
    virtual void accept( GroupVisitor* visitor ); // rcl, Sept 2005

    // given a variable, it returns the following one in the chain
    // they should belong to this section or ASSERT will be generated
    // and -1 will be returned.
    int     GetNextVariable( int iVar );

    static const int MAX_WORKSECLEN = 1000;

    // Symbol overrides
    Symbol* FindChildSymbol(const std::wstring& symbol_name) const override;

protected:
    void WriteJsonMetadata_subclass(JsonWriter& json_writer) const override;

private:
    const Logic::SymbolTable& GetSymbolTable() const;

private:
    int m_containerIndex = 0; // the container table index

public:
    int GetContainerIndex() const  			    { return m_containerIndex; }
    void SetContainerIndex(int container_index) { m_containerIndex = container_index; }
};
