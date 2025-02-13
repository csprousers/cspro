#pragma once

#include <engine/Defines.h>


struct EXP_HEADER_NODE
{
    TCHAR   m_iNodeType;
    TCHAR   m_iExportModel;              // 1:Flat, 2:Ensembled
    TCHAR   m_iExportAppLevel;           // Level where the export is executed
    TCHAR   m_iExportLevel;              // Final export level
    bool    m_bHasCaseId;                // has a declared CASE_ID

    int     m_iExportProcSymbol;
    int     m_iExportProcType;

    TCHAR   m_iLenRecId;
    TCHAR   m_iNumCaseId;          // # of vars in CASE_ID
    int     m_iLenCaseId;               // length of CASE_ID list
    int     m_iLenCaseIdUnicode;        // GHM 20130502 the length of the case IDs (if in SPSS/SAS/Stata Unicode mode, where alphas are written out with four bytes to a character)

    int     m_iRecNameExpr;
    int     m_iRecTypeExpr;
    bool    m_bIsSymbolRecName;
    bool    m_bIsSymbolRecType;
    bool    m_bCaseIdAfterRecType;
};

struct EXP_ENSEMBLE_NODE                // ENSEMBLED    // victor Dec 12, 00
{                        
    TCHAR   m_iNodeType;
    char*   m_pEndNode;                 // address of the end-of-ensemble
};


#ifndef USE_BINARY
//---------------------------------------------------------------------------
//  File name: Export.h
//
//  Description:
//          Header for engine-Export class
//
//  History:    Date       Author   Comment
//              ---------------------------
//              20 Nov 99   RHF     Basic conversion
//              15 Jun 00   vc      Customization
//              22 Jun 00   vc      Adding 'class CExportVarCateg'
//              12 Dec 00   vc      Adding 'FORMAT ensembled record' capability
//              18 Dec 00   vc      Adding named exports & multiexport capability
//
//
//---------------------------------------------------------------------------

#include <engine/Settings.h>
#include <engine/citer.h>

class CEngineArea;
class CEngineDriver;
struct EngineData;

//---------------------------------------------------------------------------
//
//  Internal definitions
//
//---------------------------------------------------------------------------

// export models
#define Exp_mENSEMBLED      1

// export atoms
#define Exp_HCODE           1
#define Exp_EOCASE          2
#define Exp_ENSEMBLE        3
#define Exp_EOENSEMBLE      4

// marks of used dicts                                  // VC Jan 30, 95
#define ExpDicTypeNone       0          // not in a dict
#define ExpDicTypeInput      1          // in the input dict
#define ExpDicTypeExternal   2          // in an external dict
#define ExpDicTypeWork       9          // in a work dict


//---------------------------------------------------------------------------
//
//  Object description
//
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//
//  class CExportVarCateg : public CObject
//
//  Description:
//      Category (one pair value-label) for a given variable
//
//  Construction/Destruction
//      CExportVarCateg             Constructor
//      ~CExportVarCateg            Destructor
//---------------------------------------------------------------------------
class CExportVarCateg : public CObject {
public:
// --- Data members --------------------------------------------------------
    CString     csTextValue;
    CString     csTextLabel;

// --- Methods -------------------------------------------------------------
public:
    CExportVarCateg()  {}
    ~CExportVarCateg() {}
};

//---------------------------------------------------------------------------
//
//  class CExport : public CObject
//
//  Description:
//      Export description
//
//  Constructor & initialization
//      CExport                     Constructor
//      Init                        Initialization
//
//  Export gate, export interface, exporting options
//      IsExportAlive               True if the export was "already open" (no matter if success or failure)
//      IsExportActive              True if the export was "normally open"
//      IsExportUnable              True if the export was "unable to open"
//      IsExportClosed              True if the export was "closed" after processing
//      SetExportInactive           Set the export to "never open"
//      SetExportActive             Set the export to "normally open"
//      SetExportUnable             Set the export to "unable to open"
//      SetExportClosed             Set the export to "closed" after processing
//      ExportOpen                  Try to open the export
//      SetExpoName                 Set the shared name for this export
//      SetExportToDat              Set the option to export the data-file
//      SetExportToSPSS             Set the option to export a SPSS description
//      SetExportToSAS              Set the option to export a SAS description
//      SetExportToSTATA            Set the option to export a STATA description
//      GetExportToDat              True if must export the data-file
//      GetExportToSPSS             True if must export the SPSS description
//      GetExportToSAS              True if must export the SAS description
//      GetExportToSTATA            True if must export the STATA description
//      ExportDescriptions          Drive the descriptions' export activity
//      ExportClose                 Close every export files, releases memory
//
//  Output files
//      <none>                      File management is encapsulated in other methods
//
//  Program-strip
//      CreateProgStrip             ?
//      DeleteProgStrip             ?
//      EnlargeProgStrip            ?
//      SetHeadNode                 ?
//      GetHeadNode                 ?
//      GetFirstNode                ?
//      GetNextNode                 ?
//      GetNodeAt                   ?
//      CanAddNode                  ?
//      GetTopNodeIndex             ?
//      AdvanceNodeIndex            ?
//
//  Record-area
//      CreateRecArea               ?
//      DeleteRecArea               ?
//      CleanRecArea                ?
//      GetRecIndex                 ?
//      SetRecIndex                 ?
//      AdvanceRecIndex             ?
//
//  Generating descriptions
//      ex_recsetup                 ?
//
//  Generating exported data
//  --- SPS description
//      SpsDescr                    ?
//      sps_variable                ?
//      sps_label                   ?
//  --- SAS description
//      SasDescr                    ?
//      sas_format                  ?
//      sas_vallabels               ?
//      sas_attrib                  ?
//      sas_missing                 ?
//      sas_misvalue                ?
//      sas_input                   ?
//      sas_inpvariable             ?
//      sas_varattrib               ?
//      sas_label                   ?
//  --- STATA description                               // RHF 15/3/99
//      StataDescr                  ?
//      stata_variable              ?
//      stata_missing               ?
//      stata_hasvaluelabel         ?
//      stata_findvalue             ?
//      stata_isvalidvalue          ?
//      stata_isvalidvalue          ?
//  --- CSPRO description
//      CsProDescr                  ?
//      cspro_variable              ?
//  --- Utility functions
//      GetPendingSlash             ?
//      ResetPendingSlash           ?
//      SetPendingSlash             ?
//      WasPendingSlash             ?
//      GenVarName                  ?
//      GetVarLabel                 ?
//      GetVarMaxOccs               ?
//      ValueHasDecimals            ?
//  --- Array of categories for one variable
//      BuildVarCategories          Create the array of categories for one variable
//      AddVarCategory              Add one category
//      GetNumVarCategories         Return the number of categories
//      GetVarCategoryAt            Get the category at a given position
//      DeleteVarCategories         Delete the array of categories
//
//  Engine links
//      SetEngineArea               Links to engine
//
//---------------------------------------------------------------------------

#define EXPORT_ITEM            1
#define EXPORT_SUBITEM        2
#define EXPORT_ITEMSUBITEM (EXPORT_ITEM|EXPORT_SUBITEM)

class CDictExport {
public:
    CDictExport() {
        m_pDataDict = NULL;
        m_pExport = NULL;
    }

    CDataDict*  m_pDataDict;
    class CExport*  m_pExport;
};


class CExport : public CObject
{
#define STATA_MAXLAB    80              // STATA max-lab // GHM 20100426 increased size

    typedef   int (CExport::*pEmsembledTrip)( int iVar, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM],  void* pInfo );

// --- Data members --------------------------------------------------------
    // --- export gate, export interface, exporting options
public:

    int                 m_iExportAlive; // export gate: 0-not yet open, 1-normally open, 2-unable to open, 3-closed
private:
    CString             m_csExpoName;
    bool                m_bDefaultName;
    bool                m_bFirstCase;// RHF Feb 03, 2005
public:
    int                 m_iFileSymbol;    // -1 for "system assigned", string expression otherwise // victor Dec 18, 00

public:
    bool                m_bToDatFile;   // export data to data-file
    bool                m_bToSPSS;      // export description to SPSS
    bool                m_bToSAS;       // export description to SAS
    bool                m_bToSTATA;     // export description to STATA
    bool                m_bToCSPRO;     // export description to CSPRO
    bool                m_bToR;

    bool                m_bToTabDelim;
    bool                m_bToCommaDelim;
    bool                m_bToSemiColonDelim;

    bool                m_bExportItemOnly;
    bool                m_bExportSubItemOnly;
    bool                m_bExportItemSubItem;

    bool                m_bExportForceANSI; // GHM 20120416
    bool                m_bCommaDecimal;

    // notes collected from exported records
    CMap<CString, LPCTSTR, CString, CString&> m_mapRecNotes;

    int                 m_iExportItem;

    // --- output files
public:
    FILE*               m_pFileDat;     // exported data-file
    bool                m_bNeedClose;
    int                 m_iExporItem;
    FILE*               m_pFileSPSS;    // SPSS codebook-file
    FILE*               m_pFileSAS;     // SAS codebook-file
    FILE*               m_pFileSTATAdct;// STATA DCT-file
    FILE*               m_pFileSTATAdo; // STATA DO-file
    FILE*               m_pFileR;
    CDictRecord*        m_pRecordCSPRO;  // CSPRO record
    int                 m_iCurOcc;
    CString             m_csLastCaseId;

    // --- program-strip
public:
    int                 m_iExpLstSlot;  // slot size for program-strip
    char*               m_pExpLstBase;  // the program-strip itself
    int                 m_iExpLstSize;  // total of bytes in program-strip
    int                 m_iExpLstNext;  // next byte free in program-strip - for compiler only

    // --- record-area
public:
    csprochar*          m_pExpRecArea;  // the record-area itself
    int                 m_iExpRecMax;   // size of record-area
    int                 m_iExpRecNext;  // next byte free in record-area

    // --- generating descriptions
public:
    EXP_HEADER_NODE*    m_pHeadNode;

    // RHF INIC Feb 15, 2005 NEW_EXPORT
private:
    CExport*            m_pNextExport;
    bool                m_bUseMergedArray;
    int                 m_iRecordNumber;

    bool                m_bNewExportRecord;
    int                 m_iRunExportRecordNumber;

public:
    void SetNextExport( CExport* pNextExport ) { m_pNextExport = pNextExport; }
    CExport* GetNextExport() { return m_pNextExport; }

    void SetRecordNumber( int iRecordNumber ) { m_iRecordNumber = iRecordNumber; }
    int  GetRecordNumber() { return m_iRecordNumber; }

    void SetUseMergedArray( bool bUseMergedArray ) { m_bUseMergedArray = bUseMergedArray; }
    bool GetUseMergedArray() { return m_bUseMergedArray; }

public:
    CArray<int,int>     m_aMergedSymbols;
    CArray<int,int>     m_aMergedOccExpr;
    CArray<int,int>     m_aMergedOccProcSymbol;   // RHF Jun 08, 2005
    CArray<int,int>     m_aMergedRecNumber;

    void                MakeMergedArray();

    // RHF END Feb 15, 2005 NEW_EXPORT

    CArray<int,int>             m_iCaseIdItems;
    CArray<int,int>             m_aCaseIdOccExpr;
    CArray<int,int>             m_aCaseIdOccExprProcSymbol;   // RHF Jun 08, 2005
    CArray<CString,CString>     m_aCaseIdOccExprString; // Futures uses

public:
    // RHF INIC Oct 07, 2004
    CArray<int,int>     m_aUsedRecords;
    CArray<int,int>     m_aSymbols;
    CArray<int,int>     m_aOccExpr;
    CArray<int,int>     m_aOccProcSymbols; // RHF Jun 08, 2005
    CArray<CString,CString>     m_aOccExprString;
    CMap<int,int,int,int> m_aOcExprForMultSubItems;
    CMap<int,int,CString,LPCTSTR> m_aOcExprStringForMultSubItems;
    // RHF END Oct 07, 2004

    int                 m_iLastItemPos;

    CMap<CString,LPCTSTR,int,int>     m_aItemNames; // The only way to check duplicated is comparing the names. We can use the dot notation and produce duplicated names


    bool                m_bPendingSlash;
    bool                m_bCaseIdReady; // case-id already generated (or not needed)
    csprochar           m_cNameSeparator;
    int                 m_iFmtCount;    // Counter for format number in SAS   (BMD 04 Mar 2004)

    csprochar           m_pszSectionCode[MAX_RECTYPECODE+1];

    // --- array of categories for one variable
private:
    std::vector<CExportVarCateg*>* m_papVarCategs;

    // --- engine links
public:
    CEngineArea*        m_pEngineArea;
    CEngineDriver*      m_pEngineDriver;
    EngineData*         m_engineData;

private:
    const Logic::SymbolTable& GetSymbolTable() const;

// --- Methods -------------------------------------------------------------
    // --- constructor & initialization
public:
    CExport() {
            Init();
    }
private:
    void    Init();

    // --- export gate, export interface, exporting options
public:
    bool    IsExportAlive()       { return( m_iExportAlive != 0 ); }
    bool    IsExportActive()      { return( m_iExportAlive == 1 ); }
    bool    IsExportUnable()      { return( m_iExportAlive == 2 ); }
    bool    IsExportClosed()      { return( m_iExportAlive == 3 ); }
    void    SetExportInactive()   { m_iExportAlive = 0; }
    void    SetExportActive()     { m_iExportAlive = 1; }
    void    SetExportUnable()     { m_iExportAlive = 2; }
    void    SetExportClosed()     { m_iExportAlive = 3; }
    bool    ExportOpen( bool* bDeleteWhenFail );
private:
    // setSettingsFlags() and exportFlagsAreOk() to be used
    // only inside ExportOpen() to help code readability and
    // self documentation
    // RCL, Jan 2005
    void    setSettingsFlags();
    bool    exportFlagsAreOk( bool* pbAllowDuplicates );
public:
    void SetExpoName( CString csFilename, bool bDefaultName )
    {
        m_csExpoName = csFilename;
        m_bDefaultName = bDefaultName;
    }
    bool    HasDefaultName() const  { return m_bDefaultName; }
    CString GetExpoName() const     { return  m_csExpoName; } // victor Dec 12, 00
    CString GetDcfExpoName() const;
    void    RemoveFiles();

    bool    HasRecType( EXP_HEADER_NODE* pHeadNode);

    void    ExportRecType( bool bCopyToRecArea );
    int     ExportRecTypeVarName();

    void    ExportCaseId();
    int     ExportCaseIdVarNames(bool bCopyToRecArea);

    void    GetRecNameAndOcc( CString& csRecordName, int& iMaxOcc );
    void    CopyToExportArea( const TCHAR* pBuff, int iSize );
    int     CopyToExportAreaExtraChar();
    bool    MakeDefaultCaseId( int iUntilLevel );
    void    MakeCommonRecord( CDictRecord* pDictCommonRecord, int iFromLevel, int iToLevel );

    void    SetExportToDat( bool bX )   { m_bToDatFile = bX; }
    void    SetExportToSPSS( bool bX )  { m_bToSPSS = bX; }
    void    SetExportToSAS( bool bX )   { m_bToSAS = bX; }
    void    SetExportToSTATA( bool bX ) { m_bToSTATA = bX; }
    void    SetExportToCSPRO( bool bX )  { m_bToCSPRO = bX; }
    void    SetExportToTabDelim( bool bX )  { m_bToTabDelim = bX; }
    void    SetExportToCommaDelim( bool bX )  { m_bToCommaDelim = bX; }
    void    SetExportToSemiColonDelim( bool bX )    { m_bToSemiColonDelim = bX; }
    void    SetExportToR( bool bX )                 { m_bToR = bX; }

    void    SetExportItemOnly( bool bX )        { m_bExportItemOnly = bX; }
    void    SetExportSubItemOnly( bool bX )        { m_bExportSubItemOnly = bX; }
    void    SetExportItemSubItem( bool bX )        { m_bExportItemSubItem = bX; }

    void    SetExportForceANSI(bool b) { m_bExportForceANSI = b; } // GHM 20120416
    void    SetExportCommaDecimal(bool b) { m_bCommaDecimal = b; }

    bool    GetExportToDat() const              { return m_bToDatFile; }
    bool    GetExportToSPSS() const             { return m_bToSPSS; }
    bool    GetExportToSAS() const              { return m_bToSAS; }
    bool    GetExportToSTATA() const            { return m_bToSTATA; }
    bool    GetExportToCSPRO() const            { return m_bToCSPRO; }
    bool    GetExportToTabDelim() const         { return m_bToTabDelim; }
    bool    GetExportToCommaDelim() const       { return m_bToCommaDelim; }
    bool    GetExportToSemiColonDelim() const   { return m_bToSemiColonDelim; }
    bool    GetExportToR() const                { return m_bToR; }

    bool    Export4ByteUnicodeAlphas() const
    {
        return !GetExportForceANSI() && ( GetExportToSPSS() || GetExportToSAS() || GetExportToSTATA() );
    }

    bool    UseSeparator() const;
    TCHAR   GetSeparatorChar() const; // 0 if none

    bool    GetExportItemOnly()                 { return m_bExportItemOnly; }
    bool    GetExportSubItemOnly()              { return m_bExportSubItemOnly; }
    bool    GetExportItemSubItem()              { return m_bExportItemSubItem; }

    bool    GetExportForceANSI() const          { return m_bExportForceANSI; } // GHM 20120416
    bool    GetExportCommaDecimal() const       { return m_bCommaDecimal; }

    void    ExportDescriptions();
    void    ExportClose();

    bool    ExportAddDictRecord(CDataDict* pDataDict, CString csFileName,  CString& csErrorMsg );

    static  CDataDict*  ExportGetDataDict( CString csDataFileName, CMap<CString,LPCTSTR, CDictExport,CDictExport&>& aDataDicts, CExport* pExport );

    static void         ExportGenRecNameAndType( CDataDict* pDataDict );

    // --- program-strip
public:
    bool    CreateProgStrip();
    void    DeleteProgStrip();
    bool    EnlargeProgStrip();
    bool    SetHeadNode();
    EXP_HEADER_NODE* GetHeadNode()          { return m_pHeadNode; }
    char*   GetFirstNode();
    char*   GetNextNode()                   { return( m_pExpLstBase + m_iExpLstNext ); }
    char*   GetNodeAt( int iNodeIndex )     { return( m_pExpLstBase + iNodeIndex ); }
    bool    CanAddNode( int iNodeSize )     { return( m_iExpLstNext + iNodeSize < m_iExpLstSize ); }
    int     GetTopNodeIndex()               { return m_iExpLstNext; }
    void    AdvanceNodeIndex( int iSize )   { m_iExpLstNext += iSize; }

    // --- record-area
public:
    bool    CreateRecArea();
    void    DeleteRecArea();
    void    CleanRecArea( int iSize = 0 );
    int     GetRecIndex()                   { return m_iExpRecNext; }
    void    SetRecIndex( int iSize )        { m_iExpRecNext = iSize; }
    void    AdvanceRecIndex( int iSize )    { m_iExpRecNext += iSize; }

    // --- Iterator methods
public:
#ifdef _DEBUG
    int Export_PrintNames( int iVar, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM], void* pInfo=NULL );
#endif

    int EnsembledTrip( pEmsembledTrip pExportFun, bool bEvaluateItem, void* pInfo=NULL, CArray<int,int>* pExtSymbols=NULL, CArray<int,int>* pExtOccExpr=NULL, CArray<int,int>* pExtOccExprProcSymbol=NULL );

    int Export_GetRecordLen( int iVar, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM], void* pInfo=NULL  );
    int Export_GenRecordList( int iVar, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM], void* pInfo=NULL  );

    //Data
    int Export_Data( int iVar, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM], void* pInfo=NULL  );
    int Export_Names( int iVar, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM], void* pInfo=NULL  );

    //CsPro
    int Export_CsPro( int iVar, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM], void* pInfo=NULL  );

    //Spss&Stata
    int Export_NumRecords( int iVar, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM], void* pInfo=NULL  );

    //Spss
    int Export_SpssDescription( int iVar, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM], void* pInfo=NULL  );
    int Export_SpssVarLabel( int iVar, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM], void* pInfo=NULL  );
    int Export_SpssMissing( int iVar, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM], void* pInfo=NULL  );
    int Export_SpssValueLabels( int iVar, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM], void* pInfo=NULL  );

    //Sas
    int Export_SasFormat( int iVar, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM], void* pInfo=NULL  );
    int Export_SasAttrib( int iVar, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM], void* pInfo=NULL  );
    int Export_SasMissing( int iVar, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM], void* pInfo=NULL );
    int Export_SasInput( int iVar, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM], void* pInfo=NULL  );

    //Stata
    int Export_StataDescription( int iVar, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM], void* pInfo=NULL  );
    int Export_StataVarLabel( int iVar, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM], void* pInfo=NULL  );
    int Export_StataValueLabels( int iVar, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM], void* pInfo=NULL  );
    int Export_StataAsocValueLabels( int iVar, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM], void* pInfo=NULL  );

    //CsPro
    int Export_CsProDescription( int iVar, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM], void* pInfo=NULL  );

    // R
    int Export_R_Format( int iVar, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM], void* pInfo=NULL );
    int Export_R_Names( int iVar, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM], void* pInfo=NULL );
    int Export_R_ValueLabels( int iVar, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM], void* pInfo=NULL );

    // --- generating descriptions

    // --- SPS description
public:
    int     SpsDescr( int iNumRecords );
private:
    void    sps_variable( VART* pVarT, const CString& csExportedVarname, int & iLoc );
    void    sps_label( csprochar* pszGenLabel, csprochar* pszLabel );

    // --- SAS description
public:
    void    SasDescr();
private:
    void    sas_format( CMap<VART*,VART*,int,int>* pMapSasLabel );
    void    sas_vallabels( VART* pVarT, CMap<VART*,VART*,int,int>* pMapSasLabel );
    void    sas_attrib( CMap<VART*,VART*,int,int>* pMapSasLabel );
    void    sas_missing();
    void    sas_misvalue( VART* pVarT, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM] );
    int     sas_input( int iRecLen );
    void    sas_inpvariable( VART* pVarT, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM], int iLoc );
    void    sas_varattrib( VART* pVarT, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM], CMap<VART*,VART*,int,int>* pMapSasLabel );
    void    sas_label( csprochar* pszGenLabel, csprochar* pszLabel );

    // --- STATA description                            // RHF 15/3/99
public:
    int     StataDescr( int iNumRecords );
private:
    void    stata_variable( VART* pVarT, const CString& csExportedVarName, int iLoc, int iRecNum );
    bool    stata_hasmissing( VART* pVarT );
    bool    stata_missing( VART* pVarT );
    bool    stata_hasvaluelabel( VART* pVarT );
    bool    stata_findvalue( VART* pVarT, double dValue );
    bool    stata_isvalidvalue( csprochar* pszValue );
    bool    stata_isvalidvalue( double dValue );
    int     stata_countValidValues();

    // --- CSPRO description
public:
    int     CsProDescr();
private:
    void    cspro_variable( VART* pVarT, const CString& csExportedVarname, int iLoc );

    // --- R description
public:
    void    R_Descr(); // GHM 20120507

    // --- utility functions
    // RHF INIC Feb 03, 2005
public:
    //Flag to export item names only in the first line
    bool                m_bNamesExported;
    bool    GetNamesExported()      { return m_bNamesExported; }
    void    SetNamesExported( bool bNamesExported )   { m_bNamesExported = bNamesExported; }

    //Remove file at the end when there are errors
    bool                m_bRemoveFiles;
    bool    GetRemoveFiles()        { return m_bRemoveFiles; }
    void    SetRemoveFiles( bool bRemoveFiles )   { m_bRemoveFiles = bRemoveFiles; }
    // RHF END Feb 03, 2005

private:
    bool    GetPendingSlash()       { return m_bPendingSlash; }
    void    ResetPendingSlash()     { m_bPendingSlash = false; }
    void    SetPendingSlash()       { m_bPendingSlash = true; }
    bool    WasPendingSlash();

    void    GenVarName( CString& csVarName, VART* pVarT, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM] );
    void    GenOccName( CString& csOccName, csprochar cSeparator, bool bUseFirstSeparator, VART* pVarT, int aIndex[DIM_MAXDIM], int aDimFlag[DIM_MAXDIM] );

    void    GetVarLabel( VART* pVarT, TCHAR* pszLabel );
    int     GetVarMaxOccs( VART* pVarT );
    bool    ValueHasDecimals( double dValue );

    // --- array of categories for one variable
private:
    int     BuildVarCategories( VART* pVarT );
    int     SASBuildVarCategories( VART* pVarT );
    bool    AddVarCategory( CExportVarCateg* pVarCateg );
    int     GetNumVarCategories() const;
    CExportVarCateg* GetVarCategoryAt( int iValue );
    void    DeleteVarCategories();

    void    CopyQuotedStringWithLengthLimit(TCHAR* pDest, const TCHAR* pSource);

    // --- engine links
public:
    void    SetEngineArea( CEngineArea* pEngineArea );
};

#endif
