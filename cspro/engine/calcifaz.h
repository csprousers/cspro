#pragma once

#ifdef USE_BINARY
#else

#include <engine/Batdrv.h>
#include <engine/BATIFAZ.H>

class CCalcIFaz : public CBatchIFaz {
public:
    CCalcIFaz();

    // CsCalc stuff
    // For the application
    int  C_GetAppCtabs( CArray <CTAB*, CTAB*>& aCtabs );

    // For Each TBD
    bool C_OpenInputTbd( CString csInputTbdName );
    void C_CloseInputTbd();

    int C_GetTbdCtabs( CArray<CTAB*,CTAB*>& aUsedCtabs,
                       CArray<CTAB*,CTAB*>& aUnUsedCtabs,
                       CArray<CTbdTable*,CTbdTable*>& aTables );


    int C_GetTbdBreakKeys( CStringArray& aBreakKey, CUIntArray& aBreakNumKeys );

    // Between TBD, XTS & APP. IF Tbd has less tables than the APP tables
    // show a warning (show error if it not a subset).
    bool C_CheckIntegrity( CStringArray& csErrorMsg );

    //For each break in the current TBD
    bool C_LoadBreak( CString csCurrentBreakKey, int iBreakKeyNum, CArray<CTAB*, CTAB*>* aUsedCtabs, int iCurrentBreak, int iNumBreaks );

    void C_SetRunTimeBreakKeys( CStringArray* aBreakKeys, CUIntArray*  aBreakNumKeys, CArray<CTAB*, CTAB*>* aUsedCtabs );
    void SetProcessSpcls4Tab(bool bFlag) { m_bTabProcessSpecial = bFlag;}
};

#endif // USE_BINARY
