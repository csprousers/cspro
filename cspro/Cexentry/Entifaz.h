#pragma once
//---------------------------------------------------------------------------
//  File name: Entifaz.h
//
//  Description:
//          Header for interface to engine
//
//  History:    Date       Author   Comment
//              ---------------------------
//              15 Jul 99   RHF     Created
//              09 Aug 00   RHF     Add Insert Case method
//              09 Aug 00   RHF     Add Insert/Delete/Reorder group ocurrences method
//              30 Sep 00   vc      Add methods for moving to previous persistent fields
//              10 May 01   vc      Adding primary CsDriver (see Handshaking3d marks)
//              25 Jul 01   vc      Adding support to manage Enter-flow operations
//
//---------------------------------------------------------------------------

// C_GotoField actions
#define ENGINE_NEXTFIELD    1
#define ENGINE_BACKFIELD    2

#include <engine/DEFLD.H>
#include <engine/Entdrv.h>
#include <zCapiO/capi.h>
#include <Zissalib/CsDriver.h>

enum class SpecialFunction : int;


class CEntryIFaz
{
// --- Data members --------------------------------------------------------
private:
    // --- basic data
    CEngineDriver*  m_pEngineDriver;
    CEntryDriver*   m_pEntryDriver;
    CEngineArea*    m_pEngineArea;
    EngineData*     m_engineData;
    CIntDriver*     m_pIntDriver;
    CSettings*      m_pEngineSettings;
    CsDriver*       m_pCsDriver;                        // victor Apr 27, 01

    bool            m_bExentryStarted;
    bool            m_bExentryInited;
    bool            m_bModifyStarted;

    // --- CAPI support
    CCapi*          m_pCapi;                            // RHF Jan 13, 2000

// --- Methods -------------------------------------------------------------
    // --- construction/destruction/initialization
public:
    CEntryIFaz();
    virtual ~CEntryIFaz();
    void DeleteEntryDriver();
    void SetEntryDriver( CEntryDriver* pEntryDriver );


    // --- from WEXENTRY.CPP
public:
    bool    C_ExentryStart( int iExMode );
    bool    C_ExentryInit( CNPifFile* pPifFile, int* iExMode );
    bool    C_ExentryInit1( CNPifFile* pPifFile, int* iExMode );//SAVY 03 May 2002
    void    C_ExentryStop();
    void    C_ExentryEnd( int bCanExit );
    bool    C_IsNewCase();


    // --- for CsDriver only                            // victor Dec 10, 01
public:
    void    C_ResetDoorCondition();
    DEFLD*  C_BlindNodeAdvance( bool bStopOnNextNode, int iStopNode );


    // --- field functions
public:
    DEFLD*  C_FldGetCurrent();
public:
    DEFLD_INFO C_FldInfo(const DEFLD* pDeFld);
    void    C_FldPutVal(DEFLD* pDeFld, CString csValue, bool* value_has_been_modified = nullptr);
    CString C_FldGetVal(DEFLD3* pDeFld);
private:
    VARX*   C_FldEvaluateFieldAndIndices(DEFLD* pDeFld, CNDIndexes& theIndex);
    CString FormatEnteredValue(const DEFLD* pDeFld, CString entered_value);


    // --- other functions
public:
    bool    C_IsPathOn();
    int     C_GetStatus( const DEFLD* pDeFld );

    int     C_GetStatus3( const DEFLD3* pDeFld3 );            // RHF Jul 31, 2000
    int     C_GetGroupOcc( int Var );
    CSettings* GetSettings() { return m_pEngineSettings; }


    // --- methods coming from ISSAW
public:
    DEFLD*  C_GoToField( int iAction );
    bool    C_ModifyStart( int bDoInitFile );
    void    C_ModifyStop();
    bool    C_SetStopNode(int iNode);


    // --- CAPI support
public:
    CCapi*  C_GetCapi();

    // --- new methods added for IMSA-CsPro
public:
    DEFLD*  C_EndGroup( bool bPostProc );
    DEFLD*  C_MoveToField( DEFLD* pTargetFld, bool bPostProc );
    DEFLD*  C_MoveToBlock(int block_symbol_index, bool bPostProc);
    DEFLD*  C_AdvancePastBlock();

    bool    C_IsNewNode();
    DEFLD*  C_EndLevel( bool bPostProcCurField, bool bPostProcAllOthers, int iNextLevelToCapture, bool bWriteNode, bool bAutoEndLevel );
    DEFLD*  C_EndGroupOcc( bool bPostProc );     // as requested by Glenn Mar 30, 00
    int     C_GetCurrentLevel();

    CEntryDriver*   GetEntryDriver() { return m_pEntryDriver; } // RHF Jul 31, 2000

    // --- group occurrences manipulation               // RHF Aug 09, 2000
public:
    DEFLD*  C_InsertOcc( bool& bRet );
    DEFLD*  C_DeleteOcc( bool& bRet );
    DEFLD*  C_SortOcc( const bool bAscending, bool& bRet );
    DEFLD*  C_InsertOccAfter( bool& bRet );             // victor Mar 26, 02
private:
    DEFLD*  C_MoveToField( DEFLD* pTargetFld, bool bPostProc, bool bAllowProtected, bool bAllowReenterToNoLight );
    DEFLD*  C_MoveOcc( bool& bRet, GROUPT::eGroupOperation eOperation, bool bAscending = false );

public:
    DEFLD*  C_PreviousPersistentField(); // victor Sep 30, 00

    // --- miscellaneous methods
public:
    bool    C_IsAutoEndGroup(); // RHF Nov 07, 2000

public:
    int     C_GetMaxNumLevel();

    bool    HasSpecialFunction(SpecialFunction special_function);
    double  ExecSpecialFunction(int iVar, SpecialFunction special_function, double argument);
    void    RunGlobalOnFocus(int iVar);

    C3DObject* CallCsDriverBrain();               // victor Dec 10, 01
private:
    DEFLD3* GetFieldForInterface();               // victor Dec 10, 01

public:
    // RHF INIC Nov 08, 2002
    bool SetCurrentLanguage(wstring_view language_name);
    std::vector<Language> GetLanguages(bool include_only_capi_languages = true) const;
    // RHF END Nov 08, 2002

    CDEFormFile* GetFormFileInProcess();
    CDEFormFile* GetPrimaryFormFile();
    bool         InEnterMode();
    int          GetNumLevels( bool bPrimaryFlow ); // If not primary, current
    CString      GetCurrentKey( int iLevel );
    int          GetCurrentLevel( DICT** pDicT=NULL );

    bool         QidReady( int iLevel );

    int          GetVariableSymbolIndex(CString csFullName);

    void         ToggleCapi( int iSymVar );

    VARX*        GetVarX( int iVar );


    int         C_GetKeyLen( const CDataDict* pDataDict, int iLevel ); // RHF Jul 21, 2003

    void        C_SetOperatorId(CString csOperatorID);
    CString     C_GetOperatorId() const;

    bool        C_HasSomeRequest();// RHF Nov 06, 2003
    DEFLD*      C_RunCsDriver( bool bCheckRange );// RHF Nov 06, 2003

private:
    const Logic::SymbolTable& GetSymbolTable() const;
};
