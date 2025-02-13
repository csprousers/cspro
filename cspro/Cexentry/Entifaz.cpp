#include "STDAFX.H"
#include "Entifaz.h"
#include <engine/3dException.h>
#include <engine/Engine.h>
#include <engine/Exappl.h>
#include <engine/ProgramControl.h>
#include <engine/runmodes.h>
#include <zEngineO/Block.h>
#include <zToolsO/Tools.h>
#include <zUtilO/TraceMsg.h>
#include <zFormO/FormFile.h>
#include <zFormO/Roster.h>
#include <ZBRIDGEO/npff.h>
#include <Zissalib/CFlAdmin.h>
#include <Zissalib/groupt2.h>


#if defined(_DEBUG) && defined(WIN_DESKTOP)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
//
// --- construction/destruction/initialization
//
/////////////////////////////////////////////////////////////////////////////

// RHF INIC Jan 13, 2000
CEntryIFaz::CEntryIFaz() {
    m_pEngineDriver    = NULL;
    m_pEntryDriver     = NULL;
    m_pEngineArea      = NULL;
    m_engineData       = nullptr;
    m_pIntDriver       = NULL;
    m_pEngineSettings  = NULL;
    m_pCsDriver        = NULL;

    m_bExentryStarted  = false;
    m_bExentryInited   = false;
    m_bModifyStarted   = false;

    m_pCapi            = new CCapi;
}

CEntryIFaz::~CEntryIFaz() {
    DeleteEntryDriver();
    delete m_pCapi;
}


const Logic::SymbolTable& CEntryIFaz::GetSymbolTable() const
{
    return m_engineData->symbol_table;
}


void CEntryIFaz::SetEntryDriver( CEntryDriver* pEntryDriver ) {
    m_pEntryDriver    = pEntryDriver;
    m_pEngineArea     = m_pEntryDriver->m_pEngineArea;
    m_engineData      = &m_pEngineArea->GetEngineData();
    m_pEngineDriver   = m_pEntryDriver;
    m_pEngineSettings = m_pEngineDriver->m_pEngineSettings;
    m_pIntDriver      = m_pEngineDriver->m_pIntDriver.get();

    m_pIntDriver->Enable3D_Driver();    // switching to 3D-driver
    m_pCapi->SetEntryDriver( m_pEntryDriver );
    m_pEntryDriver->SetCapi( m_pCapi ); // RHF Nov 29, 2002
}

void CEntryIFaz::DeleteEntryDriver() {
    if( m_pEntryDriver != NULL )
        delete m_pEntryDriver;

    m_pEntryDriver    = NULL;
    m_pEngineArea     = NULL;
    m_pEngineDriver   = NULL;
    m_pEngineSettings = NULL;
    m_pIntDriver      = NULL;
    m_pCsDriver = NULL;
}
// RHF END Jan 13, 2000



/////////////////////////////////////////////////////////////////////////////
//
// --- from WEXENTRY.CPP
//
/////////////////////////////////////////////////////////////////////////////

bool CEntryIFaz::C_ExentryStart( int iExMode ) {    // must be called from interface
    bool    bDone = false;

    if( m_bExentryStarted )
        return  bDone;
    m_bExentryStarted = true;

    m_pEntryDriver->reset_lastopenlevels();

    m_pIntDriver->m_bStopProc = false;

    ExMode = ( iExMode == CRUNAPL_ADD ) ? _T('A') : ( iExMode == CRUNAPL_MODIFY || iExMode == CRUNAPL_VERIFY ) ? _T('U') : _T(' ');

    m_pEntryDriver->exentrystart();     // initializes sections and open LST

    // insure the Primary Flow is set "in process"        // victor Jan 06, 00
    ASSERT( m_pEntryDriver->GetFlAdmin() != 0 );
    m_pCsDriver = m_pEntryDriver->GetFlAdmin()->GetCsDriver();  // victor Jul 25, 01

    m_pEntryDriver->SetModificationMode(false);
    m_pEntryDriver->SetInsertMode(false);
    m_pEntryDriver->SetVerifyMode(false);


    // checking files depending on "mode"
    if( iExMode == CRUNAPL_ADD ) {
        m_pEntryDriver->Issademode = ADD;
        bDone = true;
    }
    else { // iExMode == CRUNAPL_MODIFY || iExMode == CRUNAPL_VERIFY
        m_pEntryDriver->Issademode = ADD;

        if( iExMode == CRUNAPL_VERIFY )
            m_pEntryDriver->SetVerifyMode( true );

        // Para que el UPDATE se comporte como ADD
        ExMode = _T('A');
        m_pEntryDriver->SetModificationMode( true );

        bDone = true;
    }

    if( !bDone )
        C_ExentryStop();

    return  bDone;
}

bool CEntryIFaz::C_ExentryInit( CNPifFile* pPifFile, int* iExMode ) {
    bool    bDone = false;

    if( m_bExentryInited )
        return bDone;

    CEntryDriver* pEntryDriver = new CEntryDriver(pPifFile->GetApplication(), this);

    SetEntryDriver( pEntryDriver );

    m_bExentryInited = true;

    Issamod = ModuleType::Entry;

    m_pEngineDriver->SetPifFile( pPifFile );            // see dictload.cpp

    bDone = m_pEngineDriver->exapplinit();

    if( bDone )
    {
        *iExMode = ExMode;

        DIX(0)->entermode = true;

        m_pEntryDriver->Exit_Code = 0;
        m_pEntryDriver->ResetStopCode();
    }

    return  bDone;
}

//SAVY Added the following function . Called when compile is not needed . 03 May 2002
/////////////////////////////////////////////////////////////////////////////////
//
//      bool CEntryIFaz::C_ExentryInit1( CNPifFile* pPifFile, int* iExMode )
//
/////////////////////////////////////////////////////////////////////////////////
bool CEntryIFaz::C_ExentryInit1( CNPifFile* pPifFile, int* iExMode )
{
    bool    bDone = false;

    m_bExentryInited = true;

    Issamod = ModuleType::Entry;

    m_pEngineDriver->SetPifFile( pPifFile );            // see dictload.cpp

    bDone = m_pEngineDriver->exapplinit();

    if( bDone )
    {
        *iExMode = ExMode;

        m_pEntryDriver->Exit_Code = 0;
        m_pEntryDriver->ResetStopCode();
    }

    return  bDone;
}

void CEntryIFaz::C_ExentryStop() {      // close Externals DAT/IDX, and LST
    if( !m_bExentryStarted )
        return;

    // RHF INIC Dec 21, 2000
    CFlAdmin*   pFlAdmin = m_pEntryDriver->GetFlAdmin(); // victor Aug 08, 01

    pFlAdmin->ReactivatePrimary();                  // victor Aug 08, 01

    // (re)install the CsDriver currently active    // victor Aug 02, 01
    m_pCsDriver = pFlAdmin->GetCsDriver();          // victor Aug 02, 01

    m_pCsDriver->InitSessionConditions();
    // RHF END Dec 21, 2000

    m_pEntryDriver->ResetEnterMode(); // RHF Sep 13, 2001

    m_pEntryDriver->doend();                            // RHF 20/9/99

    m_pEngineDriver->ResetDynamicAttributes();

    m_bExentryStarted = false;

    m_pEntryDriver->Exit_Code = m_pEntryDriver->GetStopCode().value_or(0);
    m_pEntryDriver->ResetStopCode();

    m_pEngineDriver->CloseListerAndWriteFiles();

    m_pEntryDriver->SetModificationMode(false);
    m_pEntryDriver->SetInsertMode(false);
    m_pEntryDriver->SetVerifyMode(false);

#ifdef WIN_DESKTOP
    if( m_pCapi ) {                                     // RHF Jan 30, 2000
        m_pCapi->DeleteLabels();                        // RHF Jan 30, 2000
    }                                                   // RHF Jan 30, 2000
#endif
}


void CEntryIFaz::C_ExentryEnd( int bCanExit ) {
    if( !m_bExentryInited )
        return;

    m_bExentryInited = false;

    m_pEngineDriver->CloseRepositories(true);

#ifdef USE_BINARY
    if( Dicxbase != 0 )
#else
    ASSERT( Dicxbase != 0 );
#endif

    m_pIntDriver->StopApplication();

    m_pEngineArea->tablesend();

    int exit_code = 0;
    if( bCanExit )
        exit_code = m_pEntryDriver->Exit_Code;

    DeleteEntryDriver();

    if( bCanExit )
        exit( exit_code );
}


bool CEntryIFaz::C_IsNewCase() {
    return( m_bExentryInited ? m_pEntryDriver->IsNewCase() : false );
}



/////////////////////////////////////////////////////////////////////////////
//
// --- for CsDriver only                                // victor Dec 10, 01
//
/////////////////////////////////////////////////////////////////////////////

void CEntryIFaz::C_ResetDoorCondition() {
    CFlAdmin*   pFlAdmin = m_pEntryDriver->GetFlAdmin();

    pFlAdmin->ReactivatePrimary();
    m_pCsDriver = pFlAdmin->GetCsDriver();
    m_pCsDriver->ResetDoorCondition();
    m_pCsDriver->ResetSourceOfNodeAdvance();            // victor Feb 23, 02
}

DEFLD* CEntryIFaz::C_BlindNodeAdvance( bool bStopOnNextNode, int iStopNode ) {
    ASSERT( m_pCsDriver != NULL );
    DEFLD*      pReachedFld = m_pCsDriver->GetCurDeFld();

    // remark - bStopOnNextNode true means "advance to next node", otherwise is "to the end of current case"
    if( bStopOnNextNode ) {
        m_pCsDriver->SetInterRequestNature( CsDriver::AdvanceTo );
        m_pCsDriver->Set3DTarget( NULL );
    }
    else {
        if( iStopNode >= 0 )
            m_pCsDriver->SetTargetNode( iStopNode );
        else
            m_pCsDriver->SetTargetNode( CEntryDriver::NodeAdvanceToEndOfCase );
    }
    m_pCsDriver->SetInterRequestNature( CsDriver::NextField );

    // asks CsDriver to solve the request
    CallCsDriverBrain();

    pReachedFld = GetFieldForInterface();

    return pReachedFld;
}

/////////////////////////////////////////////////////////////////////////////
//
// --- field functions
//
/////////////////////////////////////////////////////////////////////////////

DEFLD* CEntryIFaz::C_FldGetCurrent()
{
    return m_pCsDriver ? m_pCsDriver->GetCurDeFld() : NULL;
}

DEFLD_INFO CEntryIFaz::C_FldInfo(const DEFLD* pDeFld)
{
    DICT*   pDicT = NULL;
    SECT*   pSecT = NULL;
    VART*   pVarT = NULL;
    int     iSymFrm = 0;
    int     iSymVar = pDeFld->GetSymbol();

    if( iSymVar > 0 ) {
        pVarT = VPT(iSymVar);
        pSecT = pVarT->GetSPT();

        if( pSecT != NULL )
            pDicT = pSecT->GetDicT();

        iSymFrm = pVarT->SYMTfrm;
    }

    DEFLD_INFO DeFldInfo;
    DeFldInfo.vp = pVarT;

    if( pDicT != NULL )
        DeFldInfo.dname = WS2CS(pDicT->GetName());

    if( pSecT != NULL ) {
        DeFldInfo.sname = WS2CS(pSecT->GetName());
        DeFldInfo.level = pSecT->GetLevel();
    }

    if( iSymFrm > 0 )
        DeFldInfo.fname = WS2CS(NPT(iSymFrm)->GetName());

    if( pVarT != NULL )
        DeFldInfo.vname = WS2CS(pVarT->GetName());

    return DeFldInfo;
}

VARX* CEntryIFaz::C_FldEvaluateFieldAndIndices(DEFLD* pDeFld, CNDIndexes& theIndex)
{
    ASSERT(theIndex.isZeroBased());

    if( pDeFld == NULL ) {
        pDeFld = m_pCsDriver->GetCurDeFld();
    }

    if( pDeFld == NULL )
        return nullptr;

    int iSymVar = pDeFld->GetSymbol();

    if( iSymVar <= 0 )
        return nullptr;

    if( pDeFld->getIndexValue(0) <= 0 )                      // RHF 19/8/99
        pDeFld->setIndexValue( 0, 1 );                       // RHF 19/8/99

    VARX* pVarX = VPX(iSymVar);

    int iOccur = -345; // Magic value to recognize later if it has been correctly assigned

    if( pDeFld->isUsingOnlyOneDimension() )
        iOccur = pDeFld->getIndexValue(0);

    else
    {
        ASSERT( !pDeFld->isUsingOnlyOneDimension() );

        // TODO: Use the complete values and do not calculate
        //       anything here...
        //       Probably theIndex = *pDeFld ???

        VART* pVarT = pVarX->GetVarT();
        int iNumDim = pVarT->GetNumDim();
        if( iNumDim > 0 )
        {
            // will consider last valid dimension
            //        We suppose that the last valid dimension is the one
            //        we should use, is this correct?

            iOccur = pDeFld->getIndexValue( pVarT->GetDimType( iNumDim - 1 ) );
        }
        else
        {
            iOccur = pDeFld->getIndexValue( 0 );
        }
    }

    pVarX->BuildIntParentIndexes( theIndex, iOccur );

    return pVarX;
}


CString CEntryIFaz::FormatEnteredValue(const DEFLD* pDeFld, CString entered_value)
{
    // format an entered value (in the format that the interface provides)
    // to get it in a format that can be accepted by the engine
    const VART* pVarT = VPT(pDeFld->GetSymbol());

#ifdef WIN_DESKTOP
    if( entered_value.GetLength() < (int)pVarT->GetDictItem()->GetCompleteLen() )
        entered_value = CIMSAString::MakeExactLength(entered_value, (int)pVarT->GetDictItem()->GetCompleteLen());

    // process numerics with decimals
    if( pVarT->IsNumeric() && pVarT->GetDecimals() > 0 )
    {
        int complete_length = (int)pVarT->GetDictItem()->GetCompleteLen();
        int decimal_position = complete_length - pVarT->GetDecimals() - 1;
        ASSERT(entered_value[decimal_position] == _T('.'));

        bool value_was_entered = _istdigit(entered_value[decimal_position - 1]);

        // ensure that a number or negative sign appears before the decimal mark
        if( !value_was_entered && _istdigit(entered_value[decimal_position + 1]) )
        {
            if( entered_value[decimal_position - 1] == _T(' ') )
                entered_value.SetAt(decimal_position - 1, _T('0'));

            else
                ASSERT(entered_value[decimal_position - 1] == _T('-'));

            value_was_entered = true;
        }

        // if a value has been entered, zero fill the decimal portion of the number
        if( value_was_entered )
        {
            for( int i = complete_length - 1; entered_value[i] == _T(' '); i-- )
                entered_value.SetAt(i, _T('0'));
        }

        // if nothing has been entered, then remove the decimal mark
        if( pVarT->GetDecChar() && !value_was_entered )
            entered_value.SetAt(decimal_position, _T(' '));

        // remove a decimal mark from implicit values
        else if( !pVarT->GetDecChar() )
            entered_value = entered_value.Left(decimal_position) + entered_value.Mid(decimal_position + 1);
    }
#endif

    // in the portable environments, the value is formatted in CoreEntryPageField methods
    ASSERT(entered_value.GetLength() == pVarT->GetLength());

    return entered_value;
}


void CEntryIFaz::C_FldPutVal(DEFLD* pDeFld, CString csValue, bool* value_has_been_modified/* = nullptr*/)
{
    CNDIndexes theIndex(ZERO_BASED);
    VARX* pVarX = C_FldEvaluateFieldAndIndices(pDeFld, theIndex);

    if( pVarX != nullptr )
    {
        csValue = FormatEnteredValue(pDeFld, csValue);
        pVarX->SetValue(theIndex, csValue, value_has_been_modified);
    }
}


CString CEntryIFaz::C_FldGetVal(DEFLD3* pDeFld)
{
    DEFLD3 DeField3;

    if( pDeFld == nullptr )
    {
        pDeFld = m_pCsDriver->GetCurDeFld();

        if( pDeFld == nullptr || !GetEntryDriver()->MakeField3(&DeField3, pDeFld) )
            return _T("");
    }

    else
        DeField3 = *pDeFld;

    VART* pVarT = VPT(DeField3.GetSymbol());
    VARX* pVarX = pVarT->GetVarX();
    CNDIndexes theIndex = DeField3.getIndexes();

    return pVarX->GetValue(theIndex);
}

/////////////////////////////////////////////////////////////////////////////
//
// --- other functions
//
/////////////////////////////////////////////////////////////////////////////

bool CEntryIFaz::C_IsPathOn() {
    return m_pEngineSettings->IsPathOn();
}

int CEntryIFaz::C_GetStatus3( const DEFLD3* pDeFld3 ) {
    // hint: GetFieldColor returns one of these values (see DEFINES.h):
    //          FLAG_NOLIGHT    0x00    never entered
    //          FLAG_MIDLIGHT   0x01    grey (PathOn) or yellow (PathOff)
    //          FLAG_HIGHLIGHT  0x02    green (highlight)
    //
    ASSERT( pDeFld3->GetSymbol() > 0 );
    VARX*   pVarX = VPX( pDeFld3->GetSymbol() );

    CNDIndexes theIndex = pDeFld3->GetIndexes();

    int     iStatus = m_pIntDriver->GetFieldColor( pVarX, theIndex );

    return iStatus;
}

int CEntryIFaz::C_GetStatus( const DEFLD* pDeFld ) {
    ASSERT( pDeFld );
    DEFLD3  DeField3;
    GetEntryDriver()->MakeField3( &DeField3, pDeFld );

    return C_GetStatus3( &DeField3 );
}


int CEntryIFaz::C_GetGroupOcc( int iSymVar ) { // RHF+VC Sep 02,99 // RCH March 28, 2000
    int     iGroupOcc = 0;
    GROUPT* pGroupT   = ( iSymVar > 0 ) ? VPT(iSymVar)->GetOwnerGPT() : NULL;

    if( pGroupT != NULL ) {
        CDEGroup*   pGroup = pGroupT->GetCDEGroup();

        iGroupOcc = pGroup->GetCurOccurrence();
    }

    return iGroupOcc;
}

/////////////////////////////////////////////////////////////////////////////
//
// --- methods coming from ISSAW
//
/////////////////////////////////////////////////////////////////////////////

DEFLD* CEntryIFaz::C_GoToField( int iAction ) {
    // RHF INIC May 04, 2000. COMMENT WHEN THE PROBLEM BE FIXED
    static bool    bVirgin=true;
    if( !bVirgin ) {
        ASSERT( 0 );
        return( NULL );
    }
    bVirgin = false;
    // RHF END May 04, 2000

    ASSERT( m_bExentryStarted );
    if( !m_bExentryStarted ) return NULL;               // RHF Mar 15, 2001

    DEFLD3*     pReachedFld = NULL;

    // setup NextField/PrevField request only when no request is being solved
    if( !m_pCsDriver->IsRequestValid() ) {
        // set request definition and parms
        if( iAction == ENGINE_NEXTFIELD )
            m_pCsDriver->SetInterRequestNature( CsDriver::NextField );
        else if( iAction == ENGINE_BACKFIELD )
            m_pCsDriver->SetInterRequestNature( CsDriver::PrevField );
        else
            ASSERT( 0 );                    // can't be (only ENGINE_NEXTFIELD/ENGINE_BACKFIELD allowed here)
    }

    // asks CsDriver to solve the request
    CallCsDriverBrain();

    bVirgin = true;   // TODO limpiar esto              // RHF May 04, 2000

    // FUTURE: return p3DObject;
    pReachedFld = GetFieldForInterface();               // victor Dec 10, 01

    return pReachedFld;
}

bool CEntryIFaz::HasSpecialFunction(SpecialFunction special_function)
{
    return m_pIntDriver->HasSpecialFunction(special_function);
}

double CEntryIFaz::ExecSpecialFunction(int iVar, SpecialFunction special_function, double argument)
{
    return m_pIntDriver->ExecSpecialFunction(iVar, special_function, { argument });
}

// RHF INIC Aug 22, 2002
void CEntryIFaz::RunGlobalOnFocus( int iVar ) {
    m_pIntDriver->RunGlobalOnFocus( iVar );
}
// RHF END Aug 22, 2002

bool CEntryIFaz::C_ModifyStart( int bDoInitFile ) {
    ASSERT( m_bExentryStarted );
    if( !m_bExentryStarted ) return false; // RHF Mar 15, 2001

    if( m_bModifyStarted )
        return false;

    if( !m_pEntryDriver->IsModification() )
        return false;                   // only for UPDATE mode

    m_bModifyStarted = true;

    m_pEntryDriver->reset_lastopenlevels();

    // starting modify' environment
    m_pEntryDriver->corr_init();

    if( bDoInitFile )
        m_pEntryDriver->DoQid();

    m_pEntryDriver->Decorrlevl = 0;
    if( !bDoInitFile )                                  // victor Jan 28, 00
        m_pEntryDriver->Decorrlevl = 1;                 // victor Jan 28, 00

    DICT*   pDicT   = DIP(0);             // referred to input dict only
    int     iSymSec = pDicT->SYMTfsec;

    while( iSymSec > 0 ) {
        m_pEngineDriver->initsect( iSymSec );
        iSymSec = SPT(iSymSec)->SYMTfwd;
    }

    return true;
}

void CEntryIFaz::C_ModifyStop() {
    ASSERT( m_bExentryStarted );
    if( !m_bExentryStarted ) return; // RHF Mar 15, 2001

    if( !m_bModifyStarted )
        return;

    if( !m_pEntryDriver ) // RHF Feb 14, 2003
        return;// RHF Feb 14, 2003

    if( !m_pEntryDriver->IsModification() )
        return;                         // only for UPDATE mode

    // ending modify' environment
    m_bModifyStarted = false;

#ifdef WIN_DESKTOP
    if( m_pCapi ) {                                     // RHF Jan 30, 2000
        m_pCapi->DeleteLabels();                        // RHF Jan 30, 2000
    }                                                   // RHF Jan 30, 2000
#endif
}


// uNode == INT_MAX for advance to end of case
bool CEntryIFaz::C_SetStopNode(int iNode)
{
    m_pEntryDriver->iModification_Node = iNode;

    ASSERT( m_pCsDriver != NULL );
    int iTargetNode = ( iNode >= 0 && iNode < CEntryDriver::NodeAdvance::NodeAdvanceToNextNode ) ? iNode : -1;
    m_pCsDriver->SetTargetNode( iTargetNode );

    return true;
}

/////////////////////////////////////////////////////////////////////////////
//
// --- CAPI support
//
/////////////////////////////////////////////////////////////////////////////

// RHF INIC Jan 13, 2000
CCapi* CEntryIFaz::C_GetCapi() {
    //  ASSERT( m_bExentryStarted );
    return  m_pCapi;
}
// RHF END Jan 13, 2000


//-------------------- comments only <begin> -------------------------
//  - Interfaz selecciona modo UPDATE en vez de ADD
//  - Interfaz pone caja para capturar variables de la clave (debe sacar las
//    variables de identificacion y sus largos desde el diccionario
//  - Una vez entrados los campos en la interfaz se llama a la funcion
//    int C_ModifyGetKeys( csprochar* pszKeyPrefix, csprochar* pszKeysFileName) la cual almacena en el
//    archivo pszKeysFileName todas las claves que calzan con el prefijo recien ingresado.
//    Esta funcion retorna el numero de claves: 0,.., n o -1 si hay error.
//  - Una vez seleccionada la clave, se llama a la funcion:
//    TREE_NODE* C_ModifyGetNodes( long lFpos, int* pNumNodes ) la cual entrega un
//    arbol con los nodos del lFpos. En caso de error se retorna NULL
//  - Con los nodos la interfaz dibuja el arbol y acepta 3 posibles acciones:
//    DELETE, MODIFY y ADDSON. Para ello se llama a la funcion:
//
//
//-------------------- comments only <end> ---------------------------

/////////////////////////////////////////////////////////////////////////////
//
// --- new methods added for IMSA-CsPro
//
/////////////////////////////////////////////////////////////////////////////

DEFLD* CEntryIFaz::C_EndGroup( bool bPostProc ) {
    ASSERT( m_bExentryStarted );
    if( !m_bExentryStarted ) return NULL; // RHF Mar 15, 2001
    bool    bCanUseEnd  = m_pEngineSettings->IsPathOff();


#ifdef BUCEN
        if(true) {
        // setup source of movement
        int     iSymSourceGroup =0;
        int     iSymSourceVar = -1;
        DEFLD*  pReachedFld = NULL ;
        pReachedFld = m_pCsDriver->GetCurDeFld();
        if(pReachedFld){
             iSymSourceVar = pReachedFld->GetSymbol();
             iSymSourceGroup = m_pEngineArea->GetGroupOfSymbol( iSymSourceVar );

        }

        if(GPT(iSymSourceGroup)) {
            CDEGroup*   pGroup = GPT(iSymSourceGroup)->GetCDEGroup();
            if(pGroup) {
                CDERoster* pRoster = DYNAMIC_DOWNCAST(CDERoster,pGroup);
                if(pRoster && pRoster->UsingFreeMovement()){
                    bCanUseEnd = true;
                    bPostProc = true;
                }
            }
        }
    }
#endif

    C3DObject   o3DSource;
    C3DObject*  p3DObject;

    m_pCsDriver->FurnishCurObject( &o3DSource );

    if( !bCanUseEnd )
        p3DObject = &o3DSource;
    else {
        // set request definition and parms
        m_pCsDriver->SetInterRequestNature( CsDriver::InterEndGroup );
        m_pCsDriver->AtEndGroup_SetEnterField( bPostProc );

        // asks CsDriver to solve the request
        p3DObject = CallCsDriverBrain();
    }

    // FUTURE: return p3DObject;
    DEFLD*  pReachedFld = m_pCsDriver->GetCurDeFld();
    return pReachedFld;
}

// public
DEFLD* CEntryIFaz::C_MoveToField( DEFLD* pTargetFld, bool bPostProc ) {
    return C_MoveToField( pTargetFld, bPostProc, false, false );
}

// private
DEFLD* CEntryIFaz::C_MoveToField( DEFLD* pTargetFld, bool bPostProc, bool bAllowProtected, bool bAllowReenterToNoLight ) {
    // adding 'bPostProc' as requested by Glenn:        // victor Apr 05, 00
    // * 3.b. add 'bPostProc' parameter to MoveToField, // victor Apr 05, 00
    //        defaulted to 'true' so as not to          // victor Apr 05, 00
    //        interfere with existing code              // victor Apr 05, 00
    ASSERT( m_bExentryStarted );
    if( !m_bExentryStarted ) return NULL; // RHF Mar 15, 2001

    DEFLD*      pReachedFld = m_pCsDriver->GetCurDeFld();
    C3DObject   o3DSource;

    m_pCsDriver->FurnishCurObject( &o3DSource );

    // check source & target: both should be a Var with form
    int         iSymSource = o3DSource.GetSymbol();
    int         iSymTarget = ( pTargetFld != NULL ) ? pTargetFld->GetSymbol() : 0;

    bool        bSourceOK = ( iSymSource > 0 && NPT(iSymSource)->IsA(SymbolType::Variable) && VPT(iSymSource)->IsInAForm() );
    bool        bTargetOK = ( iSymTarget > 0 && NPT(iSymTarget)->IsA(SymbolType::Variable) && VPT(iSymTarget)->IsInAForm() );

    if( !bSourceOK || !bTargetOK )
        return pReachedFld;

    C3DObject o3DTarget( iSymTarget );

    // RHF INIC Jun 19, 2001

    if( pTargetFld->isUsingOnlyOneDimension() )
    {
        VARX* pVarX = VPX(iSymTarget);

        CNDIndexes theIndex( ZERO_BASED );

        int iTargetOccur = pTargetFld->getIndexValue(0);
        if( iTargetOccur < 1 )
            iTargetOccur = 1;

        pVarX->BuildIntParentIndexes( theIndex, iTargetOccur );

        VART* pVarT = pVarX->GetVarT();
        for( int i = 0; i < pVarT->GetNumDim(); i++ )
        {
            CDimension::VDimType vType = pVarT->GetDimType(i);

            o3DTarget.setIndexValue( vType, theIndex.getIndexValue(vType) + 1 );
        }

        // o3DTarget.setIndexes( theIndex );
    }
    else
    {
        ASSERT( pTargetFld != 0 );
        ASSERT( !pTargetFld->isUsingOnlyOneDimension() );

        o3DTarget = *pTargetFld;
    }


    // moves to the requested target
    C3DObject*  p3DObject = &o3DSource;
    int         iLocation = m_pCsDriver->SearchTargetLocation( &o3DTarget ); // victor Dec 10, 01

    if( iLocation ) {
        bool    bPathOff = m_pEngineSettings->IsPathOff();
        bool    bShouldReenter = ( iLocation < 0 );

        // moves according to target location
        if( bShouldReenter )            // ... use normal reenter
            m_pCsDriver->SetInterRequestNature( CsDriver::Reenter );
        else if( bPathOff )             // ... PathOff: use "skip to"
            m_pCsDriver->SetInterRequestNature( CsDriver::SkipTo );
        else                            // ... PathOn: use "advance to"
            m_pCsDriver->SetInterRequestNature( CsDriver::AdvanceTo );

        m_pCsDriver->Set3DTarget( &o3DTarget );

        // asks CsDriver to solve the request
        bool remake_origin_value = m_pCsDriver->RemakeOrigin();
        ASSERT(remake_origin_value); // this gets set to true above, a new change on
                                     // 20181210 to fix a problem with moving to a field
                                     // on a block (via the case tree or F6) ... this ASSERT
                                     // is here, to be removed at some point, so that we
                                     // can learn about all the situations in which this
                                     // would have been false, and to see if this change
                                     // causes any errors

        p3DObject = CallCsDriverBrain();
    }

    // FUTURE: return p3DObject;
    pReachedFld = GetFieldForInterface();               // victor Dec 10, 01

    return pReachedFld;
}


DEFLD* CEntryIFaz::C_MoveToBlock(int block_symbol_index, bool bPostProc)
{
    // this method is used to handle + skips in operator-controlled mode
    C3DObject o3DTarget(block_symbol_index);
    m_pCsDriver->SetInterRequestNature(CsDriver::SkipTo);
    m_pCsDriver->Set3DTarget(&o3DTarget);
    CallCsDriverBrain();
    return GetFieldForInterface();
}


DEFLD* CEntryIFaz::C_AdvancePastBlock()
{
    DEFLD* pReachedFld = m_pCsDriver->GetCurDeFld();
    ASSERT(pReachedFld != nullptr);

    VART* pVarT = VPT(pReachedFld->GetSymbol());
    const EngineBlock* engine_block = pVarT->GetEngineBlock();
    ASSERT(engine_block != nullptr && engine_block->GetFormBlock().GetDisplayTogether());

    // keep moving to the next field as long as the next field remains part of the block and hasn't been visited
    std::set<int> visited_field_indices;
    visited_field_indices.insert(pReachedFld->GetSymbol());

    CNDIndexes original_index = pReachedFld->GetIndexes();

    while( ( pReachedFld = C_GoToField(ENGINE_NEXTFIELD) ) != nullptr )
    {
        int field_symbol = pReachedFld->GetSymbol();

        if( engine_block->ContainsField(field_symbol) )
        {
            if( visited_field_indices.find(field_symbol) == visited_field_indices.end() )
            {
                // this is a different field on the block, so check if the occurrence has changed
                if( original_index == pReachedFld->GetIndexes() )
                {
                    visited_field_indices.insert(field_symbol);
                    continue;
                }
            }
        }

        break;
    }

    return ( pReachedFld != nullptr ) ? GetFieldForInterface() : nullptr;
}


// RHF INIC Feb 15, 2000
bool CEntryIFaz::C_IsNewNode()
{
    bool    bIsNewNode = false;

    // get node' condition
    int     iActiveLevel   = m_pEntryDriver->GetActiveLevel();
    int     iNodeCondition = 1;         // assuming "empty node, no child"

    if( iActiveLevel > 0 )
    {
        return m_pEntryDriver->IsLevelNodeNew(iActiveLevel);


        // REPO_TEMP is any of the below old code needed?

        // get node source                      <begin> // victor Mar 22, 01
        bool    bFromFile = ( m_pEntryDriver->LevCtGetSource( iActiveLevel ) == CEntryDriver::FromFile );

        if( bFromFile )
            return( bIsNewNode );
        // get node source                      <end>   // victor Mar 22, 01

        iNodeCondition = m_pEntryDriver->EvaluateNode( iActiveLevel );

        // extracting condition components
        int     iOwnStatus = -1;        // about this node
        int     iSonStatus = -1;        // about his immediate son
        bool    bWrittenChildren = ( iNodeCondition == 0 );

        if( !bWrittenChildren ) {
            if( iNodeCondition < 10 ) {
                iOwnStatus = iNodeCondition;
                iSonStatus = 0;         // no son
            }
            else {
                iOwnStatus = iNodeCondition / 10;
                iSonStatus = iNodeCondition % 10;
            }
        }

        // choose return value based three condition components:
        //
        // ... bWrittenChildren: true or false
        //
        // ... iOwnStatus:
        //     -1 ... has written children (bWrittenChildren is true)
        //      1 ... an empty node: it has no field entered
        //      2 ... a virgin node: the only fields entered are either Persistent or Protected
        //      3 ... a non-virgin node: there is at least one (not Persistent nor Protected) field entered
        //
        // ... iSonStatus:
        //     -1 ... Evaluated node has written children (or any descendant)
        //      0 ... Evaluated node has no son
        //      1 ... Evaluated node has an empty son-node
        //      2 ... Evaluated node has a virgin son-node
        //      3 ... Evaluated node has a non-virgin son-node

        // IsNewNode if the evaluated node is empty or virgin and if the node
        // doesn't have any kind of son.
        bIsNewNode = ( ( iOwnStatus == 1 || iOwnStatus == 2 ) && iSonStatus == 0 );
    }

    return bIsNewNode;
}

DEFLD* CEntryIFaz::C_EndLevel( bool bPostProcCurField, bool bExecProcAllOthers, int iNextLevel, bool bWriteNode, bool bAutoEndLevel ) {
    if( m_pEntryDriver == NULL )
        return NULL;

    ASSERT( m_bExentryStarted );
    if( !m_bExentryStarted ) return NULL; // RHF Mar 15, 2001

    if( !bAutoEndLevel )
        ASSERT( !bExecProcAllOthers );      // provisional agreement// Feb 29, 00

    bool        bCanUseEnd  = m_pEngineSettings->CanUseEndLevel();

    if( bAutoEndLevel )
        bCanUseEnd = true;

    bool        bRestackUpToEndOfNode = false;
    bool        bAdvanceToEndOfNode   = false;
    C3DObject   o3DSource;
    C3DObject*  p3DObject;

    m_pCsDriver->FurnishCurObject( &o3DSource );

    if( !bCanUseEnd ) {
        // error - cannot request EndLevel, it's disabled
        issaerror( MessageType::Error, 91601 );

        p3DObject = &o3DSource;
    }
    else {
        // checking consistency of requested parameters
        bool    bProblemFound = true;
        int     iMaxLevel     = m_pCsDriver->GetMaxLevel();
        int     iCurLevel     = m_pCsDriver->GetLevelOfCurObject();
        GROUPT* pGroupTRoot   = m_pCsDriver->GetGroupTRoot();
        int     iSymCurLevel  = pGroupTRoot->GetLevelSymbol( iCurLevel );
        int     iSymNextLevel = 0;
        int     iSymSonLevel  = 0;
        csprochar    pszCurLevel[512];
        csprochar    pszNextLevel[512];
        csprochar    pszSonLevel[512];

        m_pEntryDriver->LevCtResetForcedNextLevel();
        m_pEntryDriver->LevCtResetForcedNoWrite( iCurLevel );

        if( iNextLevel <= 0 ) {
            // error - cannot request an "stop" action by means of EndLevel
            issaerror( MessageType::Error, 91602, iNextLevel );
        }
        else if( iNextLevel > iMaxLevel ) {
            // error - cannot request a NextLevel beyond limits
            issaerror( MessageType::Error, 91603, iNextLevel, iMaxLevel );
        }
        else if( iNextLevel > iCurLevel + 1 ) {
            // error - requesting to "skip" one intermediate level
            issaerror( MessageType::Error, 91604, iNextLevel, iCurLevel );
        }
        else {
            int     iNodeCondition = m_pEntryDriver->EvaluateNode( iCurLevel );

            iSymNextLevel = pGroupTRoot->GetLevelSymbol( iNextLevel );
            iSymSonLevel  = ( iCurLevel + 1 <= iMaxLevel ) ? pGroupTRoot->GetLevelSymbol( iCurLevel + 1 ) : 0;

            // setup names of current and requested next levels
            _stprintf( pszCurLevel,  _T("%d (%ls/%ls)"), iCurLevel, NPT(iSymCurLevel)->GetName().c_str(), GPT(iSymCurLevel)->GetLabel().GetString() );
            _stprintf( pszNextLevel, _T("%d (%ls/%ls)"), iNextLevel, NPT(iSymNextLevel)->GetName().c_str(), GPT(iSymNextLevel)->GetLabel().GetString() );
            if( iSymSonLevel )
                _stprintf( pszSonLevel, _T("%d (%ls/%ls)"), iCurLevel + 1, NPT(iSymSonLevel)->GetName().c_str(), GPT(iSymSonLevel)->GetLabel().GetString() );
            else
                *pszSonLevel = 0;

            if( iNodeCondition == 0 ) {
                // error - has written children
                issaerror( MessageType::Error, 91607, pszCurLevel );
                //bProblemFound = false;
            }
            else if( iNodeCondition > 10 ) {
                // error - has a son in process
                int     iSonStatus = iNodeCondition % 10;
                const csprochar*   pszSonStatus = ( iSonStatus == 1 ) ? _T("an empty")     :
                                       ( iSonStatus == 2 ) ? _T("a virgin")     :
                                       ( iSonStatus == 3 ) ? _T("a non-virgin") : _T("a probably empty");

                issaerror( MessageType::Error, 91608, pszCurLevel, pszSonStatus, pszSonLevel );
            }
            else if( !bWriteNode ) {
                if( m_pEntryDriver->LevCtGetSource( iCurLevel ) == CEntryDriver::FromFile ) {
                    // error - requesting no-write, but the node comes from file
                    issaerror( MessageType::Error, 91605, pszCurLevel );
                }
                else if( iNextLevel > iCurLevel ) {
                    // error - no-write AND a son requested
                    issaerror( MessageType::Error, 91606, pszCurLevel, pszSonLevel );
                }
                else {
                    // no-write AND next-level brother/parent/ascendant: is OK
                    bProblemFound = false;
                }
            }
            else {
                // remainder combinations should be OK
                bProblemFound = false;
            }

            // final checkings before proceed
            if( !bProblemFound ) {          // issuing warnings on task to be made
                int     iMessage = 0;       // 9161x/9162x/9163x set below
                bool    bNextIsBrother = false;

                if( !bWriteNode ) {
                    // ... forcing to abandon current node and open a new...
                    if( iNextLevel == iCurLevel ) {
                        iMessage = 91612;   // ... brother-node
                        bNextIsBrother = true;
                    }
                    else if( iNextLevel == iCurLevel - 1 )
                        iMessage = 91613;   // ... parent-node
                    else
                        iMessage = 91614;   // ... ascendant-node
                }
                else if( !bExecProcAllOthers ) {
                    // ... forcing to restack up to end-of-node and open a new...
                    bRestackUpToEndOfNode = true;

                    if( iNextLevel > iCurLevel )
                        iMessage = 91621;   // ... son-node
                    else if( iNextLevel == iCurLevel ) {
                        iMessage = 91622;   // ... brother-node
                        bNextIsBrother = true;
                    }
                    else if( iNextLevel == iCurLevel - 1 )
                        iMessage = 91623;   // ... parent-node
                    else
                        iMessage = 91624;   // ... ascendant-node
                }
                else {
                    // ... forcing to advance up to end-of-node and open a new...
                    bAdvanceToEndOfNode = true;

                    if( iNextLevel > iCurLevel )
                        iMessage = 91631;   // ... son-node
                    else if( iNextLevel == iCurLevel ) {
                        iMessage = 91632;   // ... brother-node
                        bNextIsBrother = true;
                    }
                    else if( iNextLevel == iCurLevel - 1 )
                        iMessage = 91633;   // ... parent-node
                    else
                        iMessage = 91634;   // ... ascendant-node
                }

                // warning of effective request to be honored
                //BMD May 4, 2000 if( bNextIsBrother )
                //BMD May 4, 2000    issaerror( MessageType::Warning, iMessage, pszNextLevel );
                //BMD May 4, 2000 else
                //BMD May 4, 2000    issaerror( MessageType::Warning, iMessage, pszNextLevel, pszCurLevel );

                // looking for "forced-next-level" condition
                //  a) ... when target is an ascendant, or
                //  b) ... when target is same level, but not the maximum descendant
                bool    bForced = ( ( iNextLevel < iCurLevel ) ||
                                    ( iNextLevel == iCurLevel && iNextLevel < iMaxLevel ) );

                if( bForced )               // ... mark forced-next-level, or
                    m_pEntryDriver->LevCtSetForcedNextLevel( iNextLevel );
                else                        // ... follow the natural hierarchy
                    m_pEntryDriver->LevCtResetForcedNextLevel();

                // looking for "forced-no-write" condition
                if( !bWriteNode ) {
                    m_pEntryDriver->LevCtSetForcedNoWrite( iCurLevel );

                    // REPO_TEMP: is this a place to delete a node? see and delete below
#ifdef OLD
                    // UPDATE mode: mark node as deleted        // victor May 31, 00
                    if( m_pEntryDriver->IsModification() )      // victor May 31, 00
                        m_pEntryDriver->DataMapDeleteCurNode(); // victor May 31, 00
#endif
                }
            }
        }
        if( bProblemFound )
            bCanUseEnd = false;             // locally disabling EndLevel
    }

    // getting requested action done
    if( bCanUseEnd ) {
        // set request definition and parms
        m_pCsDriver->SetInterRequestNature( CsDriver::InterEndLevel );
        m_pCsDriver->AtEndLevel_SetParameters( bPostProcCurField, iNextLevel, bWriteNode );

        // asks CsDriver to solve the request
        if( !bAutoEndLevel )
           p3DObject = CallCsDriverBrain();
    }

    if( bAutoEndLevel )
        return NULL;

    // FUTURE: return p3DObject;
    DEFLD*  pReachedFld = m_pCsDriver->GetCurDeFld();
    return pReachedFld;
}

DEFLD* CEntryIFaz::C_EndGroupOcc( bool bPostProc ) {    // as requested by Glenn Mar 30, 00
    ASSERT( m_bExentryStarted );
    if( !m_bExentryStarted ) return NULL; // RHF Mar 15, 2001

    C3DObject*  p3DObject   = NULL;
    DEFLD*      pReachedFld = m_pCsDriver->GetCurDeFld();
    bool        bCanUseEnd  = m_pEngineSettings->IsPathOff();

    if( bCanUseEnd ) {
        m_pCsDriver->SetInterRequestNature( CsDriver::InterEndOccur );

        // asks CsDriver to solve the request
        p3DObject = CallCsDriverBrain();
    }

    // FUTURE: return p3DObject;
    pReachedFld = GetFieldForInterface();

    return pReachedFld;
}

int CEntryIFaz::C_GetCurrentLevel() {
    int     iActiveLevel = m_pEntryDriver->GetActiveLevel();

    return iActiveLevel;
}
// RHF END Feb 15, 2000


/////////////////////////////////////////////////////////////////////////////
//
// --- group occurrences manipulation                   // RHF Aug 09, 2000
//
/////////////////////////////////////////////////////////////////////////////

// RHF INIC Aug 09, 2000
DEFLD*  CEntryIFaz::C_InsertOcc( bool& bRet ) {
    return C_MoveOcc( bRet, GROUPT::GrouptInsertOcc );
}

DEFLD*  CEntryIFaz::C_DeleteOcc( bool& bRet ) {
    return C_MoveOcc( bRet, GROUPT::GrouptDeleteOcc );
}

DEFLD* CEntryIFaz::C_SortOcc( const bool bAscending, bool& bRet ) {
    return C_MoveOcc( bRet, GROUPT::GrouptSortOcc, bAscending );
}

DEFLD*  CEntryIFaz::C_InsertOccAfter( bool& bRet ) {    // victor Mar 26, 02
    return C_MoveOcc( bRet, GROUPT::GrouptInsertOccAfter );
}

DEFLD*  CEntryIFaz::C_MoveOcc( bool& bRet, GROUPT::eGroupOperation eOperation, const bool bAscending ) {
    bRet = false;

    bool    bInsert     = ( eOperation == GROUPT::GrouptInsertOcc );
    bool    bDelete     = ( eOperation == GROUPT::GrouptDeleteOcc );
    bool    bSort       = ( eOperation == GROUPT::GrouptSortOcc );

    // remark - full remake to meet new definitions, according to agreement of Mar 21 02 // victor Mar 25, 02
    bool    bInsAfter   = ( eOperation == GROUPT::GrouptInsertOccAfter ); // victor Mar 26, 02
    DEFLD*  pReachedFld = m_pCsDriver->GetCurDeFld();
    int     iSymVar     = ( pReachedFld != NULL ) ? pReachedFld->GetSymbol() : 0;

    if( iSymVar <= 0 )
        return NULL;                    // the request MUST be issued from a field

    VART*   pVarT       = VPT(iSymVar);
    GROUPT* pGroupT     = pVarT->GetOwnerGPT(); // GetParentGPT();

    int     iNumDim     = pVarT->GetNumDim();
    if( iNumDim <= 0 ) // quick fix to prevent incorrect access
        iNumDim = 1;

    // int     iOccur      = pReachedFld->getIndexValue(iNumDim-1);

    // 20110920 mike reported bug about this not working on multiply-occurring items
    int iOccur;
    if( pGroupT->GetDimType() == CDimension::Record )
        iOccur = pReachedFld->getIndexValue(iNumDim-1);

    else // multiple-occurring item
        iOccur = pReachedFld->getIndexValue(iNumDim);


    ASSERT( iOccur >= 1 );
    int     iDimType    = pGroupT->GetDimType();
    int     iNextOccur  = -1;

    // a) setup the requested 3D-objects to be used below
    C3DObject   o3DSource;
    C3DObject   o3DPivot;
    C3DObject   o3DTarget;
    C3DObject*  p3DSource = &o3DSource; // originating field
    C3DObject*  p3DPivot  = &o3DPivot;  // the GroupHead where to come back to
    C3DObject*  p3DTarget = &o3DTarget; // the "ideal target field"

    m_pCsDriver->FurnishCurObject( p3DSource );

    // a1) set the "ideal target field" to...
    if( bInsert ) {                     // ... the first field (current occurrence)
        m_pCsDriver->FurnishFirstField( p3DTarget );
        iNextOccur = p3DTarget->getIndexValue( iDimType );
    }
    else if( bDelete ) {                // ... the first field (previous occurrence)
        m_pCsDriver->FurnishFirstField( p3DTarget );
        iNextOccur = p3DTarget->getIndexValue( iDimType ) - 1;
        if( iNextOccur > 0 )
            p3DTarget->setIndexValue( iDimType, iNextOccur );
    }
    else if( bInsAfter ) {              // ... the first field (next occurrence) // victor Mar 26, 02
        m_pCsDriver->FurnishFirstField( p3DTarget );
        iNextOccur = p3DTarget->getIndexValue( iDimType ) + 1;
        p3DTarget->setIndexValue( iDimType, iNextOccur );
    }
    else {                              // ... the originating field (current occurrence)
        m_pCsDriver->FurnishCurObject( p3DTarget );
        iNextOccur = p3DTarget->getIndexValue( iDimType );
    }

    // a2) set the GroupHead to be reentered in step (d) below
    m_pCsDriver->FurnishGroupHead( p3DPivot );

    // (b) executing the requested occurrences-operation:
    if( bInsAfter )                                     // victor Mar 26, 02
        iOccur++;                                       // victor Mar 26, 02

    // (b1) setup the index-set of the current occurrence
    C3DIndexes  theSourceIndexes;
    ASSERT( !o3DSource.getIndexes().isZeroBased() );
    o3DSource.getIndexes().decrease( theSourceIndexes );

    // (b2) prepare the environment for looping
    // ... setup loop index
    int     iLoopIndex = 0;

    if( pVarT->IsArray() ) {            // TODO look for a smarter way to fit this into 3D!!!
        int     iDim = pVarT->GetNumDim() - 1;

        iLoopIndex = pVarT->GetDimType( iDim );
    }
    ASSERT( iLoopIndex >= 0 );

    // ... setup loop limits
    int     iMaxOcc   = pGroupT->GetMaxOccs();
    int     iFromOcc  = ( bSort ) ? 1 : iOccur;
    int     iUntilOcc = ( bSort ) ? pGroupT->GetDataOccurrences() : pGroupT->GetMaxOccs();

    // ... change limits to 0-based
    if( theSourceIndexes.isZeroBased() )
    {
        iMaxOcc   = ( iMaxOcc   > 0 ) ? iMaxOcc   - 1 : 0;
        iFromOcc  = ( iFromOcc  > 0 ) ? iFromOcc  - 1 : 0;
        iUntilOcc = ( iUntilOcc > 0 ) ? iUntilOcc - 1 : 0;
    }

    CLoopControl    cLoopControl( iLoopIndex, iFromOcc, iUntilOcc, iMaxOcc );

    // (b3) perform requested operation
    if( bInsert )
        bRet = pGroupT->InsertOcc( theSourceIndexes, &cLoopControl );
    else if( bDelete )
        bRet = pGroupT->DeleteOcc( theSourceIndexes, &cLoopControl );
    else if( bSort )
        bRet = pGroupT->SortOcc( theSourceIndexes, &cLoopControl, iSymVar, bAscending );
    else if( bInsAfter )                                    // victor Mar 26, 02
        bRet = pGroupT->InsertOcc( theSourceIndexes, &cLoopControl );
    else
        ASSERT(0);

    // (c) reenter the group-head
    m_pCsDriver->SetInterRequestNature( CsDriver::Reenter );

    m_pCsDriver->Set3DTarget( p3DPivot );
    CallCsDriverBrain();                // asks CsDriver to solve the request

    // (d) advance the flow to the "ideal target field"
    C3DObject*  p3DObject;
    // TODO choose a value for 'bSolveUsingCanonicAdvance' to decide how                                             // victor Mar 25, 02
    // TODO to solve this "advance", taking care of the following pros/cons:                                         // victor Mar 25, 02
    // TODO - true/using canonic advance:                                                                            // victor Mar 25, 02
    // TODO ... pros: all the procs up to the target will be fully honored                                           // victor Mar 25, 02
    // TODO ... cons: all imbedded messages are displayed, perhaps distracting the user                              // victor Mar 25, 02
    // TODO - false/using C_MoveToField:                                                                             // victor Mar 25, 02
    // TODO ... pros: uses an encapsulated method with a familiar behavior                                           // victor Mar 25, 02
    // TODO ... cons: in PathOff, it will skip instead of advance and the procs-flow will be ignored after the pivot // victor Mar 25, 02
    // TODO * suggestion: if messages-issuing is temporarily void during the 'advance', there is no difference!      // victor Mar 25, 02
    bool        bSolveUsingCanonicAdvance = true; // this is victor' advice                                          // victor Mar 25, 02
    bool        bOpenRefreshGroupOccs     = ( bInsert || bDelete || bInsAfter );
    bDelete ? bSolveUsingCanonicAdvance = false : bSolveUsingCanonicAdvance = bSolveUsingCanonicAdvance; //do not advance in the case of delete, try to stay in the current field and current occ

    if( bOpenRefreshGroupOccs )
        m_pCsDriver->SetRefreshGroupOccsLimit( 0 ); // set to "0-no limit"

    if( bSolveUsingCanonicAdvance ) {
        m_pCsDriver->SetInterRequestNature( CsDriver::AdvanceTo );
        m_pCsDriver->Set3DTarget( p3DTarget );
        p3DObject = CallCsDriverBrain();// asks CsDriver to solve the request

        // FUTURE: return p3DObject;
        pReachedFld = GetFieldForInterface();
    }
    else {
        DEFLD3  oIdealTargetFld;
        DEFLD3* pIdealTargetFld = &oIdealTargetFld;
        bool    bLightRequired  = false;

        m_pCsDriver->PassFrom3DToDeFld( p3DTarget, pIdealTargetFld, false ); // 1-index version // TRANSITION
        pReachedFld = C_MoveToField( pIdealTargetFld, false, true, !bLightRequired );
    }

    if( bOpenRefreshGroupOccs )
        m_pCsDriver->ResetRefreshGroupOccsLimit(); // reset to standard behavior

    return pReachedFld;
}


DEFLD* CEntryIFaz::C_PreviousPersistentField()
{
    DEFLD*      pReachedFld  = m_pCsDriver->GetCurDeFld();
    int         iSymFocusVar = pReachedFld->GetSymbol(); ;
    VART*       pVarTPersistent  = ( iSymFocusVar > 0 ) ? VPT(iSymFocusVar)->SearchPreviousPersistent() : NULL;

    if( pVarTPersistent != NULL )
    {
        pVarTPersistent->SetBehavior(AsAutoSkip);

        int iSymPersistentVar = pVarTPersistent->GetSymbolIndex();

        // setup the target field for MoveToField
        DEFLD PersistentFld;
        PersistentFld.SetSymbol( iSymPersistentVar );
        PersistentFld.setIndexValue( 0, 0 );

        pReachedFld = C_MoveToField( &PersistentFld, false );
    }

    return pReachedFld;
}


/////////////////////////////////////////////////////////////////////////////
//
// --- miscellaneous methods
//
/////////////////////////////////////////////////////////////////////////////

// RHF INIC Nov 07, 2000
bool CEntryIFaz::C_IsAutoEndGroup() {
    bool    bAutoEndGroup=m_pEngineSettings->IsAutoEndGroup();

    DEFLD*  pDeFld;
    if( m_pEngineSettings->IsPathOff() && ( pDeFld = C_FldGetCurrent() ) != NULL ) {
        int     iSymVar = pDeFld->GetSymbol();

        if( iSymVar > 0 ) {
            VART*   pVarT = VPT(iSymVar);

            bAutoEndGroup = !m_pEntryDriver->IsAddedNode( pVarT->GetLevel() - 1 );
        }
    }

    return bAutoEndGroup;
}
// RHF END Nov 07, 2000


int CEntryIFaz::C_GetMaxNumLevel() {
    DICT*   pDicT=DIP(0);

    return pDicT->GetMaxLevel();
}


/////////////////////////////////////////////////////////////////////////////
//
// --- HANDSHAKING3D
//
/////////////////////////////////////////////////////////////////////////////

C3DObject* CEntryIFaz::CallCsDriverBrain() // victor Aug 08, 01
{
    // CallCsDriverBrain: ask CsDriver to solve a given request
    CFlAdmin* pFlAdmin = m_pEntryDriver->GetFlAdmin();

    while( true )
    {
        CsDriver* pOldCsDriver = m_pCsDriver;

        // solve the given request, or the request installed below
        m_pIntDriver->SetCsDriver( pOldCsDriver );

        try
        {
            C3DObject* p3DObject = pOldCsDriver->DriverBrain();

            if( !pOldCsDriver->EnterFlowJustStarted() && !pOldCsDriver->EnterFlowJustReturned() )
                return p3DObject;
        }

        catch( Fatal3DException& fe )
        {
            pOldCsDriver->SetDoorCondition( CsDriver::Closed );
            issaerror( MessageType::Error, 36000, fe.getErrorCode() );
            return NULL;
        }

        catch( const EnterFlowProgramControlException& enter_flow_program_control_exception )
        {
            ASSERT(pOldCsDriver->EnterFlowJustStarted());
            pOldCsDriver->SetEnterFlowLogicStack(enter_flow_program_control_exception);
        }

        // PARADATA_TODO: add an event for modifying the flow?

        // install the CsDriver which was just (re)activated by FlAdmin
        m_pCsDriver = pFlAdmin->GetCsDriver();
        m_pIntDriver->SetCsDriver( m_pCsDriver );

        // install a request to continue the activity

        // starting a new flow:
        if( pOldCsDriver->EnterFlowJustStarted() ) {
            // ... reset the "enter-flow started" mark
            pOldCsDriver->SetEnterFlowStarted( false );

            // ... check if searching a node in the primary flow // victor Dec 10, 01
            CsDriver*   pPrimaryCsDriver = pFlAdmin->GetPrimaryCsDriver();
            bool        bNodeSearched = pPrimaryCsDriver->SearchingTargetNode();

            // ... check presence of pending advances
            bool    bPendingAdvancesInSource = ( pOldCsDriver->GetNumOfPendingAdvances() > 0 );

            if( !bNodeSearched && !bPendingAdvancesInSource )
                m_pCsDriver->SetInterRequestNature( CsDriver::NextField );
            else                        // no target - advance to infinite
                m_pCsDriver->SetInterRequestNature( CsDriver::AdvanceTo );
        }

        // returning from an entered flow:
        else {
            // ... retrieve the way is returning from the entered flow
            bool    bEnterFlowReturnedForward = pOldCsDriver->EnterFlowReturnedForward();

            // ... retrieve the progress status of the originating atom
            bool    bBeforeInterface = m_pCsDriver->IsProgressBeforeInterface();

            if( bEnterFlowReturnedForward ) {// ... returning, forward way
                // ... takes care of the enter stack
                try
                {
                    bool bRequestIssued = m_pCsDriver->RunEnterFlowLogicStack();

                    if( !bRequestIssued )
                        m_pCsDriver->SetInterRequestNature( CsDriver::NextField );
                }

                catch( const EnterFlowProgramControlException& enter_flow_program_control_exception )
                {
                    // it's possible that an enter statement was executed when running
                    // down the enter flow logic stack
                    m_pCsDriver->SetEnterFlowLogicStack(enter_flow_program_control_exception);
                }
            }
            else {                      // ... returning, backward way
                m_pCsDriver->ClearEnterFlowLogicStack();

                // RHF INIC Mar 05, 2003
                if( !bBeforeInterface ) {
                    DEFLD3* pCurField = m_pCsDriver->GetCurDeFld();

                    if( pCurField != NULL ) {
                        int     iSymVar = pCurField->GetSymbol();

                        if( iSymVar > 0 && NPT(iSymVar)->IsA(SymbolType::Variable) ) {
                            VARX*   pVarX = VPX(iSymVar);
                            VART*   pVarT = pVarX ? pVarX->GetVarT() : NULL;
                            if( pVarT->IsProtectedOrNoNeedVerif() )
                                bBeforeInterface = true;
                        }
                    }
                }
                // RHF END Mar 05, 2003

                if( bBeforeInterface )
                    m_pCsDriver->SetInterRequestNature( CsDriver::PrevField );
                else {
                    m_pCsDriver->SetInterRequestNature( CsDriver::Reenter );
                    // RHF INIC Mar 23, 2003
                    C3DObject   o3DTarget;
                    C3DIndexes  aIndex( ONE_BASED, DEFAULT_INDEXES );

                    o3DTarget.SetSymbol( -MAXLONG );
                    o3DTarget.setIndexes( aIndex );
//                    o3DTarget.SetIndexes( aIndex );

                    m_pCsDriver->Set3DTarget( &o3DTarget );

                    // RHF END Mar 23, 2003 Reenter until a non-protected field
                }
            }
        }
    }
}

DEFLD3* CEntryIFaz::GetFieldForInterface() {       // victor Dec 10, 01
    DEFLD3*     pReachedFld = m_pCsDriver->GetCurDeFld();
    C3DObject*  p3DObject = NULL;

    if( pReachedFld == NULL ) {         // no more target field for the interface
        // ending the case
        CFlAdmin*   pFlAdmin = m_pEntryDriver->GetFlAdmin();

        pFlAdmin->ReactivatePrimary();
        m_pCsDriver = pFlAdmin->GetCsDriver();

        // --- equalizing to old code below ---
        if( m_pEntryDriver->IsModification() ) {
            m_pCsDriver->SetDoorCondition( CsDriver::Locked );

            // closing a MODIFY session
            C_ModifyStop();
        }
        else if( m_pEntryDriver->m_bMustEndEntrySession ) {
            // close the ADD session
            if( m_pCsDriver->DoorConditionIs( CsDriver::Open ) )
                m_pCsDriver->GotoLevelZeroTail();
            m_pCsDriver->SetDoorCondition( CsDriver::Locked );

            // closing an ADD session
            C_ExentryStop();
        }
        else {
            // continue the ADD session with a new case
            m_pCsDriver->InitSessionConditions();

            // return "first field in primary flow" // victor Dec 10, 01
            m_pCsDriver->SetInterRequestNature( CsDriver::NextField );
            p3DObject = CallCsDriverBrain();
            pReachedFld = m_pCsDriver->GetCurDeFld();
        }
    }

    return pReachedFld;
}


bool CEntryIFaz::SetCurrentLanguage(wstring_view language_name)
{
    ASSERT(m_pEngineDriver != nullptr && m_pEngineDriver->m_pIntDriver != nullptr);

    return m_pEngineDriver->m_pIntDriver->SetLanguage(language_name, CIntDriver::SetLanguageSource::Interface);
}

std::vector<Language> CEntryIFaz::GetLanguages(bool include_only_capi_languages/* = true*/) const
{
    ASSERT(m_pEngineDriver != nullptr && m_pEngineDriver->m_pIntDriver != nullptr);

    return m_pEngineDriver->m_pIntDriver->GetLanguages(include_only_capi_languages);
}


CDEFormFile* CEntryIFaz::GetFormFileInProcess() {
    FLOW*           pCurrentFlow = m_pEngineDriver? m_pEngineDriver->GetFlowInProcess() : NULL;
    CDEFormFile*    pCurrentFormFile=pCurrentFlow?pCurrentFlow->GetFormFile() : NULL;

    return pCurrentFormFile;
}

CDEFormFile* CEntryIFaz::GetPrimaryFormFile() {
    FLOW*          pPrimaryFlow = m_pEngineDriver ? m_pEngineDriver->GetPrimaryFlow() : NULL;
    CDEFormFile*    pPrimaryFormFile=pPrimaryFlow?pPrimaryFlow->GetFormFile() : NULL;

    return pPrimaryFormFile;
}

bool CEntryIFaz::InEnterMode() {
    FLOW*           pCurrentFlow = m_pEngineDriver ? m_pEngineDriver->GetFlowInProcess() : NULL;

    bool            bEnterMode= pCurrentFlow ? !pCurrentFlow->IsPrimary() : false;

    return bEnterMode;
}

int CEntryIFaz::GetNumLevels( bool bPrimaryFlow ) {
    FLOW*   pFlow;
    int     iNumLevels=-1;

    if( bPrimaryFlow )
        pFlow = m_pEngineDriver? m_pEngineDriver->GetPrimaryFlow() : NULL;
    else
        pFlow = m_pEngineDriver? m_pEngineDriver->GetFlowInProcess() : NULL;

    CDEFormFile*    pCurrentFormFile=pFlow?pFlow->GetFormFile() : NULL;

    if( pCurrentFormFile != NULL )
        iNumLevels = pCurrentFormFile->GetNumLevels();

    return iNumLevels;
}

int CEntryIFaz::GetCurrentLevel( DICT** pDicT ) {
    DEFLD*      pDeFld = C_FldGetCurrent();

    if( pDicT != NULL )
        *pDicT = NULL;

    if( pDeFld != NULL && pDeFld->GetSymbol() > 0 ) {
        VART*   pVarT = VPT(pDeFld->GetSymbol());

        if( pDicT != NULL )
            *pDicT = pVarT->GetDPT();
        return pVarT->GetLevel();
    }

    return -1;
}

CString CEntryIFaz::GetCurrentKey( int iLevel ) {
    CString csCurrentKey;

    DICT*   pDicT;

    int iCurrentLevel = GetCurrentLevel( &pDicT );

    if( iLevel == -1 )
        iLevel = iCurrentLevel;

    if( pDicT != NULL ) {
        csprochar   pszKey[512];

        ASSERT( iLevel >= 1 );

        int iKeyLen=pDicT->DictDoMainKey( pszKey, NULL, iLevel, iLevel );
        pszKey[iKeyLen] = 0;

        csCurrentKey = pszKey;
    }

    return csCurrentKey;
}


bool CEntryIFaz::QidReady( int iLevel ) {
    return m_pEntryDriver->QidReady( iLevel );
}


int CEntryIFaz::GetVariableSymbolIndex(CString csFullName)
{
    return m_pEngineArea->SymbolTableSearch(csFullName, { SymbolType::Variable });
}

void CEntryIFaz::ToggleCapi( int iSymVar )
{
    if( iSymVar < 0 ) // all variables
    {
        for( VART* pVarT : m_engineData->variables )
        {
            if( pVarT != NULL &&  pVarT->GetSymbolIndex() > 0 && pVarT->IsInAForm() )
                ToggleCapi(pVarT->GetSymbolIndex());
        }
    }

    else
    {
        VART* pVarT = VPT(iSymVar);

        // toggle whether or not the responses will show (if using set attributes assisted)
        pVarT->SetShowExtendedControl(!pVarT->GetShowExtendedControl());

        // set the field to the default capture type if not previously defined
        if( pVarT->GetShowExtendedControl() && ( !pVarT->GetCaptureInfo().IsSpecified() ||
                                                 pVarT->GetCaptureInfo().GetCaptureType() == CaptureType::TextBox ) )
        {
            pVarT->SetCaptureInfo(CaptureInfo::GetDefaultCaptureInfo(*pVarT->GetDictItem()));
        }
    }
}

VARX* CEntryIFaz::GetVarX( int iVar ) {
    return VPX(iVar);
}

// RHF END Nov 28, 2002

// RHF INIC Jul 21, 2003
int CEntryIFaz::C_GetKeyLen( const CDataDict* pDataDict, int iLevel ) {
    int     iDict=pDataDict->GetSymbol();

    if( iDict <= 0 )
        return -1;

    DICT*   pDicT=DPT(iDict);

    if( iLevel < 0 || iLevel >= pDicT->GetMaxLevel() )
        return -1;
    else
        return pDicT->qlen[iLevel];

}
// RHF END Jul 21, 2003


void CEntryIFaz::C_SetOperatorId(CString csOperatorID)
{
    if( m_pEntryDriver != NULL )
        m_pEntryDriver->SetOperatorId(csOperatorID);
}

CString CEntryIFaz::C_GetOperatorId() const
{
    return ( m_pEntryDriver == NULL ) ? _T("") : m_pEntryDriver->GetOperatorId();
}


// RHF INIC Nov 06, 2003
bool CEntryIFaz::C_HasSomeRequest() {
    ASSERT( m_bExentryStarted );
    if( !m_bExentryStarted ) return false; // RHF Mar 15, 2001

    CsDriver::RequestNature   xRequestNature = m_pCsDriver->GetRequestNature();

    return xRequestNature != CsDriver::None;
}

DEFLD* CEntryIFaz::C_RunCsDriver( bool bCheckRange ) {
    ASSERT( m_bExentryStarted );
    if( !m_bExentryStarted ) return NULL; // RHF Mar 15, 2001

    DEFLD*      pReachedFld = m_pCsDriver->GetCurDeFld();
    C3DObject   o3DSource;

    CsDriver::RequestNature requestNature = m_pCsDriver->GetRequestNature();
    bool isSkip = (requestNature == CsDriver::SkipTo || requestNature == CsDriver::SkipToNext);
    m_pCsDriver->AtSkip_SetParameters( !isSkip && bCheckRange, true );

    m_pCsDriver->FurnishCurObject( &o3DSource );

    C3DObject*  p3DObject = &o3DSource;
    p3DObject = CallCsDriverBrain();

    m_pCsDriver->AtSkip_SetParameters( false, false );

    // FUTURE: return p3DObject;
    pReachedFld = GetFieldForInterface();               // victor Dec 10, 01

    return pReachedFld;
}
// RHF END Nov 06, 2003
