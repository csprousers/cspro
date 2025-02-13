#pragma once

// RunAplc.h: interface for CRunAplCsCalc class.
//////////////////////////////////////////////////////////////////////

#include <zBatchO/zBatchO.h>
#include <ZBRIDGEO/runapl.h>
#include <engine/calcifaz.h>
#include <zEngineO/AllSymbolDeclarations.h>

class CTbdTable;


class ZBATCHO_API CRunAplCsCalc : public CRunApl
{
private:
    CCalcIFaz*   m_pCalcIFaz;
public:
    CRunAplCsCalc( CNPifFile* pPifFile );
    ~CRunAplCsCalc();

    void SetBatchMode( const int iBatchMode );

    bool LoadCompile();
    bool End( const bool bCanExit );


    // For the application
    int  GetAppCtabs( CArray <CTAB*, CTAB*>& aCtabs );

    // For each TBD
    bool OpenInputTbd( CString csInputTbdName );
    void CloseInputTbd();

    int  GetTbdCtabs( CArray<CTAB*,CTAB*>& aUsedCtabs,
                      CArray<CTAB*,CTAB*>& aUnUsedCtabs,
                      CArray<CTbdTable*,CTbdTable*>& aTables );


    int  GetTbdBreakKeys( CStringArray& aBreakKey, CUIntArray& aBreakNumKeys );

    // Between TBD, XTS & APP. IF Tbd has less tables than the APP tables
    // show a warning (show error if it not a subset).
    bool CheckIntegrity( CStringArray& csErrorMsg );


    //For each break in the current TBD
    bool LoadBreak( CString csCurrentBreakKey, int iBreakKeyNum, CArray<CTAB*, CTAB*>* aUsedCtabs, int iCurrentBreak, int iNumBreaks );


    void SetRunTimeBreakKeys( CStringArray* aBreakKeys, CUIntArray* aBreakNumKeys, CArray<CTAB*, CTAB*>* aUsedCtabs );
    // Compiles: PROC GLOBAL, PROC levels, PROC Tables & PROC CONSOLIDATE.
    // Executes: Level1 PostCalc, Level1 Tables PostCalc, Level1 Tables Implicit Stats,
    // Level2 Postcalc, Level2 Tables PostCalc, Level2 Tables Implicit Stats, etc..
    // No level0 is executed.
    bool Start();

    bool Stop();
public:
     //SAVY 10/15/2003 for special value processing in tabs
    void SetProcessSpcls4Tab(bool bFlag) {
        if(m_pCalcIFaz){
            m_pCalcIFaz->SetProcessSpcls4Tab(bFlag);
        }
        else {
            ASSERT(FALSE);
        }
    }

};
