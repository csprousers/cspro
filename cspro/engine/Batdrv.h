#pragma once

#ifdef USE_BINARY

#else
//---------------------------------------------------------------------------
//  File name: BatDrv.h
//
//  Description:
//          Header for CBatchDriver class
//
//  History:    Date       Author   Comment
//              ---------------------------
//              15 Nov 99   RHF     Basic conversion
//              08 May 00   vc      Adapting program strip to primary flow
//              29 Jun 00   vc      Full remake to support "implicit iterations"
//              04 Apr 01   vc      Tailoring for RepMgr compatibility (Writed... erased)
//
//---------------------------------------------------------------------------
#include <engine/Engdrv.h>
#include <engine/Tbd_save.h>
#include <engine/runmodes.h>

class Pre74_Case;

//---------------------------------------------------------------------------
//
//  class CPrSlot : public CObject                      // victor Jun 29, 00
//
//  Description:
//      Slot of batch program-strip
//
//  Construction/Destruction
//      CPrSlot                     Constructor
//      ~CPrSlot                    Destructor
//
//---------------------------------------------------------------------------

class CPrSlot
{
public:
    // the slots in program-strip are of one of following types:
    enum eSlotType  { LEVELslot = 200, GRslot, Blockslot, VAslot, GIslot, CTslot, EMPTYslot };
    // where    LEVELslot   is a Level-proc slot
    //          GRslot      is a Group-proc slot
    //          GIslot      is a group-iterator slot:
    //                      the initial slot has ProgType=PROCTYPE_PRE,
    //                      the ending slot has ProgType=PROCTYPE_POST
    //          Blockslot   is a block-proc slot
    //          VAslot      is a Variable-proc slot
    //          CTslot      is a Table-proc slot

// --- Data members --------------------------------------------------------
private:
    eSlotType   m_iSlotType;
    ProcType    m_eProcType;            // PreProc, PostProc, etc.
    int         m_iSymbol;              // iSymbol
    int         m_iSlot;                // reciprocal indexes, for GIslots only:
                                        // ... iIndex of GIslot/PROCTYPE_POST for GIslot/PROCTYPE_PRE,
                                        // ... iIndex of GIslot/PROCTYPE_PRE  for GIslot/PROCTYPE_POST,
                                        // ... -1                    for other slots

// --- Methods -------------------------------------------------------------
public:
    CPrSlot()
    {
        m_iSlotType = EMPTYslot;
        m_eProcType = ProcType::None;
        m_iSymbol   = 0;
        m_iSlot     = -1;
    }

    void        SetSlotType(eSlotType iType)    { m_iSlotType = iType; }
    void        SetProcType(ProcType proc_type) { m_eProcType = proc_type; }
    void        SetSlotSymbol(int iSymbol)      { m_iSymbol = iSymbol; }
    void        SetSlotIndex(int iSlot)         { m_iSlot = iSlot; }

    eSlotType   GetSlotType() const             { return m_iSlotType; }
    bool        IsEmptySlot() const             { return ( m_iSlotType == EMPTYslot ); }
    ProcType    GetProcType() const             { return m_eProcType; }
    int         GetSlotSymbol() const           { return m_iSymbol; }
    int         GetSlotIndex() const            { return m_iSlot; }
};



//---------------------------------------------------------------------------
//
//  class CBatchDriver : public CEngineDriver
//
//  Description:
//      Header for batch-driver management
//
//  Construction/Destruction
//      CBatchDriver                Constructor
//      ~CBatchDriver               Destructor
//
//  Program strip management
//      BatchCreateProg             Build the batch program-strip
//      CreateProgLevel             Build the program-strip partition for a given level
//      CreateProgGroup             Build the program-strip block for a given group
//      CreateProgItem              Build the program-strip for an item
//      CreateProgTable             Build the program-strip for a table
//      CreateProgSlot              Prepare & add one proc-slot for the program-strip
//      AddToBatchProg              Add a new prog-slot to the program-strip
//      GetCurBatchProg             Return a pointer to the current prog-slot in the program-strip
//      GetNextBatchProg            Advance the current prog-slot and return a pointer to
//      GetBatchProgSlotAt          Return a pointer to prog-slot for a given position in the program-strip
//      SetCurBatchProgIndex        Set the moving index to program-strip to a given value
//      GetCurBatchProgIndex        Return the moving index to program-strip
//
//  Driving batch execution
//      RunInit
//      RunDriver
//      RunGroupIterator            Drives the execution of every occurrences of a given group
//      RunGroupItems               Traverse the procs of group' members up to the tail of group' iterator
//      RunEnd
//
//  Other methods
//
//     GetTbd                       .
//
//---------------------------------------------------------------------------

class CTabSet;

class CBatchDriverBase : public CEngineDriver {
    // --- Data members --------------------------------------------------------
    // program strip management
protected:
    int     m_iBatchProgIndex;          // moving index to program-strip // formerly 'BatchNext'
    CArray<int,int> m_aCtabExecOrder; // RHF May 07, 2003

public:
    std::vector<CPrSlot*> m_apBatchProg;     // the batch program-strip itself
    int     m_iLevProg[MaxNumberLevels + 1]; // starting proc for each level

protected:
    int     m_iBatchMode;
    bool    m_bHasAnyTable;
    CTbd    m_Tbd;

public:
    CString m_csOutputTbdName;

        // --- program strip management
public:
    void    BatchCreateProg();                    // formerly 'makebprog'
protected:
    virtual void    CreateProgLevel( int iLevel )=0;
    virtual int     CreateProgGroup( int iLevel, int iSymGroup, bool bDoCreate = true )=0;
    virtual int     CreateProgItem( int iLevel, int iSymVar, bool bDoCreate = true )=0;
    virtual int     CreateProgTable( int iTable, int iCtab, bool bDoCreate = true )=0;

    int     CreateProgSlot( int iLevel, CPrSlot::eSlotType iSlotType, ProcType proc_Type = ProcType::None, int iSymbol = 0 ); // formerly 'baddprog'
    int     AddToBatchProg( CPrSlot* pPrSlot );
    CPrSlot* GetCurBatchProg( void );
    CPrSlot* GetNextBatchProg( int* pIndex = NULL );
    CPrSlot* GetBatchProgSlotAt( int iIndex );
    void    SetCurBatchProgIndex( int iIndex )  { m_iBatchProgIndex = iIndex; }
    int     GetCurBatchProgIndex( void ) const  { return m_iBatchProgIndex; }
#ifdef  _DEBUG
    void    DumpBatchProg();
#endif//_DEBUG

    int     MakeCtabExecOrder(); // RHF May 07, 2003

    // --- driving batch execution ... see wExBatch.cpp
public:
    bool    RunInit();
    virtual void RunDriver()=0;
    void    RunEnd();

public:
    CBatchDriverBase(Application* pApplication);
    virtual ~CBatchDriverBase();

    void    SetBatchMode( int iBatchMode ) { m_iBatchMode = iBatchMode; }
    int     GetBatchMode()                 { return m_iBatchMode; }
    CTbd*   GetTbd() { return( &m_Tbd ); }
    void    SetHasAnyTable( bool bHasAnyTable ) { m_bHasAnyTable = bHasAnyTable; }
    bool    GetHasAnyTable()                    { return m_bHasAnyTable; }
    int     GetNumCtabToWrite();

    void    CloseCurrentInputFile();
};

class CCalcDriver : public CBatchDriverBase {
        // --- CsCalc
private:
    CTbdFile                m_InputTbd;
    CTbiFile                m_InputTbi;
    CString                 m_csCurrentBreakKey;
    int                     m_iCurrentBreakKeyNum;
    CStringArray            m_aBreakKeys;
    CUIntArray              m_aBreakNumKeys;
    CArray<CTAB*, CTAB*>    m_aUsedCtabs;

public:
    CCalcDriver(Application* pApplication);

    void    RunDriver();

    CTbdFile*   GetInputTbd() { return( &m_InputTbd ); }
    CTbiFile*   GetInputTbi() { return( &m_InputTbi ); }
    bool        OpenInputTbd( CString csInputTbdName );
    void        CloseInputTbd();

    bool        LoadBreak( CString csCurrentBreakKey, int iBreakKeyNum, CArray<CTAB*, CTAB*>* aUsedCtabs, int iCurrentBreak, int iNumBreaks );
    void        SetCurrentBreakKey( CString csCurrentBreakKey ) { m_csCurrentBreakKey = csCurrentBreakKey; }
    CString     GetCurrentBreakKey()                            { return m_csCurrentBreakKey; }
    void        SetCurrentBreakKeyNum( int iCurrentBreakKeyNum ) { m_iCurrentBreakKeyNum = iCurrentBreakKeyNum; }
    int         GetCurrentBreakKeyNum()                            { return m_iCurrentBreakKeyNum; }
    void        SetRunTimeBreakKeys( CStringArray* aBreakKeys, CUIntArray* aBreakNumKeys, CArray<CTAB*, CTAB*>* aUsedCtabs );

    // --- program strip management
public:
    void    CreateProgLevel( int iLevel );
    int     CreateProgGroup( int iLevel, int iSymGroup, bool bDoCreate = true );
    int     CreateProgItem( int iLevel, int iSymVar, bool bDoCreate = true );
    int     CreateProgTable( int iTable, int iCtab, bool bDoCreate = true );

};


class Case;
class CaseLevel;

class CBatchDriver : public CBatchDriverBase {

public:
    CString m_csExprtab;

// --- Methods -------------------------------------------------------------
public:
    // --- construction/Destruction
    CBatchDriver(Application* pApplication);
    ~CBatchDriver();

    void    RunDriver();

    // --- driving batch execution ... see wExBatch.cpp
private:
    void    RunGroupIterator( int iSymGroup );                  // victor Jun 29, 00
    void    RunGroupItems( int iHeadIndex, int iTailIndex );    // victor Jun 29, 00

    // --- other methods
public:
    bool BatchProcessCasetainer(Case* data_case, Pre74_Case* pInputCase, const std::vector<DICX*>& batch_outputs, bool* pbWriteCase);
    bool BatchProcessCaseLevel(Case* data_case, Pre74_CaseLevel* pInputLevel, const CaseLevel& case_level, const std::vector<DICX*>& batch_outputs,
                               CArray<Pre74_CaseLevel*>& apParentLevels, bool* pbWriteCase);

    // --- program strip management
public:
    void    CreateProgLevel( int iLevel );
    int     CreateProgGroup( int iLevel, int iSymGroup, bool bDoCreate = true );
    int     CreateProgBlock( int iLevel, int iSymBlock, bool bDoCreate = true );
    int     CreateProgItem( int iLevel, int iSymVar, bool bDoCreate = true );
    int     CreateProgTable( int iTable, int iCtab, bool bDoCreate = true );
};

#endif // USE_BINARY
