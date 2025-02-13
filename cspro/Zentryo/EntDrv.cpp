//---------------------------------------------------------------------------
//  File name: EntDrv.cpp
//
//  Description:
//          Implementation for the Entry' driver
//
//  History:    Date       Author   Comment
//              ---------------------------
//              .. ... ..   ..      many, many prehistoric tailoring
//              30 May 01   vc      Adding methods to fit CsDriver behavior (see EntDrv.h & DeWrite.cpp)
//              25 Jul 01   vc      This file is created in zEntryO...
//                                  ... gathering methods from wExentry, and from deWrite (not related to pRunCase)
//                                  ... adding flow-administrator for Entry runs
//
//---------------------------------------------------------------------------
#include "StdAfx.h"
#include <zPlatformO/PlatformInterface.h>
#include <zToolsO/PointerClasses.h>
#include <zToolsO/Tools.h>
#include <zToolsO/VarFuncs.h>
#include <zUtilO/ArrUtil.h>
#include <zUtilO/TraceMsg.h>
#include <zUtilF/MsgOpt.h>
#include <zHtml/WebViewSyncOperationMarker.h>
#include <zAppO/Application.h>
#include <zMessageO/Messages.h>
#include <zDictO/ValueProcessor.h>
#include <ZBRIDGEO/npff.h>
#include <zCapiO/CapiQuestionManager.h>
#include <zCaseO/Case.h>
#include <zCaseO/CaseItemReference.h>
#include <zDataO/CaseIterator.h>
#include <zDataO/DataRepository.h>
#include <engine/Entdrv.h>
#include <engine/IntDrive.h>
#include <Zissalib/CFlAdmin.h>
#include <Zissalib/CsDriver.h>
#include <zEngineO/ResponseProcessor.h>
#include <zEngineO/ValueSet.h>
#include <zEngineF/ErrmsgDlg.h>
#include <engine/EngineQuestionnaireViewer.h>
#include <CSEntry/UWM.h>


///////////////////////////////////////////////////////////////////////////////
//
// --- constructor/destructor
//
///////////////////////////////////////////////////////////////////////////////

CEntryDriver::CEntryDriver(Application* pApplication, CEntryIFaz* pEntryIFaz)
    :   CEngineDriver(pApplication, true),
        m_pFlAdmin(0)
{
    // remake to fit 'entdrv.h' data description        // victor Jan 30, 00
    // please maintain this safe order to look for typos// victor Jan 30, 00

    // --- CEngineDriver inherited data
    m_pEngineDriver        = this;

    // --- CEntryDriver own data
    m_pEntryDriver         = this;
    m_pEntryIFaz           = pEntryIFaz;

    // --- operating controls
    SetCsDriver( NULL );                                // victor Mar 07, 02
    SetActiveLevel( 0 );                // initialize m_iActiveLevel
    Issademode             = ADD;
    Decurmode              = ADDMODE;
    m_bMustEndEntrySession = false;     // was Deend = FALSE
    iModification_Node     = -1;                        // RHF Mar 20, 2001
    Decorrlevl             = 0;
    Exit_Code              = 0;

    // --- Modify/Verify flags, plus "new case" status
    m_bModifyMode          = false;
    m_bInsertMode          = false;
    m_bVerifyMode          = false;
    SetNewCase( true );                 // was m_bNewCase = true; // victor May 30, 01

    // --- other data
    m_bFileStarted         = false;

    // --> flow-administrator for Entry runs            // victor Jul 25, 01
    m_pFlAdmin          = new CFlAdmin();
    m_pFlAdmin->SetEngineDriver( m_pEngineDriver, this );

    // --> logic' Enter() command execution
    m_pEnteredFlow      = NULL;                         // victor Jan 30, 00

    // --> CAPI support
    m_pCapi             = NULL;

    // --> partial-save
    m_ePartialMode = NO_MODE;

    // --> LevCt: controlling levels operation          // victor Feb 29, 00
    for( int iLevel = 0; iLevel < (int)MaxNumberLevels; iLevel++ )
        LevCtInitLevel( iLevel );

    m_bStopAdvance = false;
    m_bStopOnOutOfRange = true;
    m_bIgnoreWrite = false;

    m_iLastRefGroupSym = -1; // RHF Mar 05, 2002
    m_iLastRefGroupOcc = -1; // RHF Mar 05, 2002

    SetOperatorId(_T(""));

    m_pmapPersistentFields = NULL;
    m_bPersistentFieldsLastUpdatedOnInsertOrModify = false;

    m_pmapAutoIncrementFields = nullptr;
    m_pLastMappedAutoIncrementFieldsRepository = nullptr;
}


CEntryDriver::~CEntryDriver()
{
    // --> flow-administrator for Entry runs            // victor Jul 25, 01
    DestroyFlAdmin();

    delete m_pmapPersistentFields;
    delete m_pmapAutoIncrementFields;
}


void CEntryDriver::exentrystart() {     // called from C_ExentryStart
    // Initializes sections and open LST
    initwsect();
    initextdi();

    try
    {
        OpenListerAndWriteFiles();
    }

    catch( const CSProException& exception )
    {
        issaerror(MessageType::Abort, exception);
        return;
    }


    ResetCaseLevels();
}

///////////////////////////////////////////////////////////////////////////////
//
// --- CsDriver interaction                             // victor Mar 07, 02
//
///////////////////////////////////////////////////////////////////////////////

void CEntryDriver::SetCsDriver( CsDriver* pCsDriver ) {
    m_pCsDriver = pCsDriver;
}

CsDriver* CEntryDriver::GetCsDriver( void ) {
    return m_pCsDriver;
}


///////////////////////////////////////////////////////////////////////////////
//
// --- managing case-id
//
///////////////////////////////////////////////////////////////////////////////

void CEntryDriver::DoQid() {
    DICT*   pDicT = DIP(0);
    int     iSymVar;
    int     iLevelKeyLen;

    QidLength = 0;
    for( int i = 0; i < (int)MaxNumberLevels && pDicT->qloc[i] > 0; i++ ) {
         iLevelKeyLen = 0;
         int j = 0;

         while( ( iSymVar = QidVars[i][j++] ) >= 0 )
             iLevelKeyLen += VPT(iSymVar)->GetLength();
         if( iLevelKeyLen != pDicT->qlen[i] )
             issaerror( MessageType::Abort, 1024, i + 1, iLevelKeyLen, pDicT->qlen[i] );

         QidLength += pDicT->qlen[i];
    }
}


bool CEntryDriver::QidReady(int iLevel)
{
    // QidReady: verifies if Q_Id components are prepared for given level
    if( iLevel < 1 || iLevel > (int)MaxNumberLevels )
        return false;

    bool bReady = true;

    // only if there is a case-identifier
    if( QidLength > 0 )
    {
        bool bPathOff = m_pEngineSettings->IsPathOff();
        int iItem = 0;

        // checks presence of every id-field for this level
        while( bReady )
        {
            int iSymVar = QidVars[iLevel - 1][iItem++];

            if( iSymVar > 0 )
            {
                int iFieldColor = m_pIntDriver->GetFieldColor(iSymVar);

                if( bPathOff )
                {
                    bReady = ( ( iFieldColor == FLAG_HIGHLIGHT ) ||
                        ( iFieldColor != FLAG_NOLIGHT && !m_pEngineDriver->IsBlankField(iSymVar,1) ) );
                }

                else
                {
                    bReady = ( iFieldColor == FLAG_HIGHLIGHT );
                }
            }

            else
            {
                break;
            }
        }
    }

    return bReady;
}


///////////////////////////////////////////////////////////////////////////////
//
// --> flow-administrator for Entry runs
//
///////////////////////////////////////////////////////////////////////////////

void CEntryDriver::DestroyFlAdmin( void ) {
    if( m_pFlAdmin != NULL )
        delete m_pFlAdmin;
    m_pFlAdmin = NULL;
}


///////////////////////////////////////////////////////////////////////////////
//
// --> auto-save and partial-save
//
///////////////////////////////////////////////////////////////////////////////
bool CEntryDriver::IsPartial()
{
    return ( m_ePartialMode != NO_MODE );
}

// RHF INIC Mar 05, 2002
// iOcc 1 based. iDirection for future uses
bool CEntryDriver::ReportToInterface( int iSymbol, int iOcc, int iDirection, CFlowAtom::AtomType xAtomType ) {
    GROUPT*             pGroupT=NULL;
    VART*               pVarT=NULL;
    bool                bDone=false;

    if( iSymbol <= 0 ) return true;
#ifdef WIN_DESKTOP
    if( AfxGetMainWnd() == NULL )
        return bDone;
#endif
    if( NPT(iSymbol)->IsA(SymbolType::Group) )
        pGroupT = GPT(iSymbol);
    else if( NPT(iSymbol)->IsA(SymbolType::Variable) ) {
        ASSERT( iOcc >= 1 );
        pVarT = VPT(iSymbol);
    }
    else
        ASSERT(0);

    bDone = true;
    // If Group/Roster send message in Head/Tail
    // If Group/ No Roster, send message in each HtOcc
    // If Vart & sequential, send message
    if( pGroupT != NULL ) {
        bool    bIsRoster = ( pGroupT->GetCDEGroup() != NULL && pGroupT->GetCDEGroup()->GetItemType() == CDEFormBase::Roster );
        bool    bFocusGroup = ( xAtomType == CFlowAtom::AtomType::GroupHead && iDirection == 1 ||
                                xAtomType == CFlowAtom::AtomType::GroupTail && iDirection == -1 );
        if( bIsRoster ) {
            if( iSymbol != m_iLastRefGroupSym && bFocusGroup ) {
                m_iLastRefGroupSym = iSymbol;
                // TODO_PORT: Need to devise a method for posting messages to the OS UI
#ifdef WIN_DESKTOP
                AfxGetMainWnd()->SendMessage(WM_IMSA_REFRESHFORM, (long) pGroupT->GetCDEGroup() );
#endif
            }
        }
        else { // No Roster. Form multiple in depth . HtOcc always is inner!
            if( bFocusGroup || xAtomType == CFlowAtom::AtomType::HTOcc ) {
                    m_iLastRefGroupSym = iSymbol;
                // TODO_PORT: Need to devise a method for posting messages to the OS UI
#ifdef WIN_DESKTOP
                AfxGetMainWnd()->SendMessage(WM_IMSA_REFRESHFORM, (long) pGroupT->GetCDEGroup() );
#endif
                }
        }
    }
    else if( pVarT != NULL ) {
        // Check if change group (for example an enter finished)
        if( pVarT->GetOwnerGroup() != m_iLastRefGroupSym ) {
            m_iLastRefGroupSym = pVarT->GetOwnerGroup();

            ASSERT( m_iLastRefGroupSym > 0 );
            pGroupT = GPT(m_iLastRefGroupSym);
            // TODO_PORT: Need to devise a method for posting messages to the OS UI
#ifdef WIN_DESKTOP
            AfxGetMainWnd()->SendMessage(WM_IMSA_REFRESHFORM, (long) pGroupT->GetCDEGroup());
#endif
        }

        if( pVarT->IsProtectedOrNoNeedVerif() ) {
            DEFLD           DeFld;

            DeFld.SetSymbol( iSymbol );
            DeFld.setIndexValue( 0, iOcc );
            // TODO_PORT: Need to devise a method for posting messages to the OS UI
#ifdef WIN_DESKTOP
            AfxGetMainWnd()->SendMessage(WM_IMSA_REFRESHPROTECTED, (long) &DeFld );
#endif
        }
    }

    return bDone;
}
// RHF END Mar 05, 2002


bool CEntryDriver::LoadPersistentFields(std::optional<double> position_in_repository/* = std::nullopt*/)
{
    bool bFirstTimeLoading = ( m_pmapPersistentFields == NULL );

    if( bFirstTimeLoading )
        m_pmapPersistentFields = new std::map<VART*,CString>;

    // no need to load the persistent fields again if they haven't been changed by an insert call
    if( !position_in_repository.has_value() && !bFirstTimeLoading && !m_bPersistentFieldsLastUpdatedOnInsertOrModify )
        return true;

    DICT* pDicT = DIP(0);

    // set up the map of persistent fields
    if( bFirstTimeLoading )
    {
        int iSymSec = pDicT->SYMTfsec;

        while( iSymSec > 0 )
        {
            SECT* pSecT = SPT(iSymSec);
            int iSymVar = pSecT->SYMTfvar;

            while( iSymVar > 0 )
            {
                VART* pVarT = VPT(iSymVar);

                if( pVarT->IsPersistent() )
                {
                    CString csValue;

                    // load the value from the PFF's persistent map if specified
                    if( m_pPifFile != NULL )
                    {
                        CString csMappedValue = m_pPifFile->GetPersistentData(WS2CS(pVarT->GetName()));

                        if( !csMappedValue.IsEmpty() )
                        {
                            TCHAR* pszValueBuffer = csValue.GetBufferSetLength(pVarT->GetLength());

                            if( pVarT->IsAlpha() )
                            {
                                int iCharsToCopy = std::min(pVarT->GetLength(),csMappedValue.GetLength());

                                _tmemcpy(pszValueBuffer,csMappedValue,iCharsToCopy);

                                if( iCharsToCopy < pVarT->GetLength() )
                                    _tmemset(pszValueBuffer + iCharsToCopy,BLANK,pVarT->GetLength() - iCharsToCopy);
                            }

                            else // numeric
                            {
                                double dValue = wcstod(csMappedValue,NULL);
                                pVarT->dvaltochar(dValue,pszValueBuffer);
                            }

                            csValue.ReleaseBuffer();
                        }
                    }

                    m_pmapPersistentFields->insert(std::make_pair(pVarT,csValue));
                }

                iSymVar = pVarT->SYMTfwd;
            }

            iSymSec = pSecT->SYMTfwd;
        }
    }

    // get out if there are no persistent fields
    if( m_pmapPersistentFields->size() == 0 )
        return true;

    // load the persistent fields from the repository
    DataRepository* pInputRepository = GetInputRepository();
    std::unique_ptr<Case> data_case;

    try
    {
        // in insert mode, use the persistent fields for the case used for the insertion point
        if( position_in_repository.has_value() )
        {
            m_bPersistentFieldsLastUpdatedOnInsertOrModify = true;
        }

        // in add mode, use the last case in the file
        else
        {
            if( !bFirstTimeLoading && !m_bPersistentFieldsLastUpdatedOnInsertOrModify )
                return true; // no need to load the persistent fields again

            m_bPersistentFieldsLastUpdatedOnInsertOrModify = false;

            auto optional_case_key = pInputRepository->FindCaseKey(CaseIterationMethod::SequentialOrder, CaseIterationOrder::Descending);

            if( !optional_case_key.has_value() )
            {
                ASSERT(pInputRepository->GetNumberCases() == 0);
                return true;
            }

            position_in_repository = optional_case_key->GetPositionInRepository();
        }

        data_case = pInputRepository->GetCaseAccess()->CreateCase();
        pInputRepository->ReadCase(*data_case, *position_in_repository);
    }

    catch( const DataRepositoryException::Error& )
    {
        return false;
    }

    // get the persistent fields from the case
    Pre74_Case* pCase = data_case->GetPre74_Case();

    for( auto& [pVarT, value] : *m_pmapPersistentFields )
    {
        const CDictItem* dict_item = pVarT->GetDictItem();
        const CDictRecord* dict_record = dict_item->GetRecord();
        const DictLevel* dict_level = dict_record->GetLevel();

        // persistent fields can only be on the ID record
        ASSERT(dict_level->GetIdItemsRec() == dict_record);

        // get the last such level in the case
        Pre74_CaseLevel* pCaseLevel = pCase->GetLastLevel(dict_level->GetLevelNumber() + 1);

        if( pCaseLevel != NULL )
        {
            const TCHAR* pszBuffer = pCaseLevel->GetFirstFilledRecordBuffer();

            const TCHAR* pszItemBuffer = pszBuffer + dict_item->GetStart() - 1;

            value = CString(pszItemBuffer, dict_item->GetLen());
        }
    }

    return true;
}

void CEntryDriver::InitializePersistentFields(int iLevel) // iLevel is one-based
{
    if( m_pmapPersistentFields == NULL )
        return;

    if( IsModification() )
    {
        m_bPersistentFieldsLastUpdatedOnInsertOrModify = true;
        return;
    }

    for( const auto& itr : *m_pmapPersistentFields )
    {
        VART* pVarT = itr.first;

        if( pVarT->GetLevel() != iLevel )
            continue;

        pVarT->LoadPersistentValue(itr.second);
    }
}

void CEntryDriver::UpdatePersistentField(VART* pVarT)
{
    if( m_pmapPersistentFields == NULL )
        return;

    TCHAR* pszVarText = m_pEngineDriver->m_pIntDriver->GetVarAsciiAddr(pVarT);
    m_pmapPersistentFields->at(pVarT) = CString(pszVarText,pVarT->GetLength());

    if( pVarT->GetBehavior() == AsAutoSkip )
        pVarT->SetBehavior(AsProtected); // make the field protected again
}

bool CEntryDriver::HasPersistentFields() const
{
    return ( m_pmapPersistentFields != NULL ) && m_pmapPersistentFields->size() != 0;
}


bool CEntryDriver::LoadAutoIncrementFields()
{
    DICT* pDicT = DIP(0);
    DICX* pDicX = pDicT->GetDicX();

    if( !pDicX->IsDataRepositoryOpen() )
        return false;

    if( m_pLastMappedAutoIncrementFieldsRepository == &pDicX->GetDataRepository() ) // the file has already been mapped
        return true;

    SAFE_DELETE(m_pmapAutoIncrementFields);
    m_pmapAutoIncrementFields = new std::map<VART*,int64_t>;

    m_pLastMappedAutoIncrementFieldsRepository = &pDicX->GetDataRepository();

    // set up the map of auto increment fields
    struct LoadHelper
    {
        VART* _pVarT;
        int64_t _llMaxValue;
        int _iKeyOffset;
        int _iLength;

        LoadHelper(VART* pVarT)
        {
            _pVarT = pVarT;
            _llMaxValue = 0;
            _iLength = _pVarT->GetLength();

            // figure out the key offset
            _iKeyOffset = 0;

            for( const CDictItem* id_item : pVarT->GetDataDict()->GetIdItems() )
            {
                if( id_item == _pVarT->GetDictItem() )
                    break;

                _iKeyOffset += id_item->GetLen();
            }
        }
    };

    std::vector<LoadHelper> aAutoIncrements;

    int iSymSec = pDicT->SYMTfsec;

    while( iSymSec > 0 )
    {
        SECT* pSecT = SPT(iSymSec);
        int iSymVar = pSecT->SYMTfvar;

        while( iSymVar > 0 )
        {
            VART* pVarT = VPT(iSymVar);

            if( pVarT->IsAutoIncrement() )
                aAutoIncrements.push_back(pVarT);

            iSymVar = pVarT->SYMTfwd;
        }

        iSymSec = pSecT->SYMTfwd;
    }

    if( aAutoIncrements.size() > 0 )
    {
        // go through the repository keys to get the highest values
        try
        {
            CaseKey case_key;
            auto case_key_iterator = pDicX->GetDataRepository().CreateCaseKeyIterator(CaseIterationMethod::KeyOrder, CaseIterationOrder::Ascending);

            while( case_key_iterator->NextCaseKey(case_key) )
            {
                for( auto& auto_increment : aAutoIncrements )
                {
                    int64_t llThisValue = (int64_t)chartodval(case_key.GetKey().GetString() + auto_increment._iKeyOffset, auto_increment._iLength, 0);
                    auto_increment._llMaxValue = std::max(llThisValue, auto_increment._llMaxValue);
                }
            }
        }

        catch( const DataRepositoryException::Error& )
        {
            // ignore exceptions
        }

        // insert the values into the auto increment field map
        for( const auto& auto_increment : aAutoIncrements )
            (*m_pmapAutoIncrementFields)[auto_increment._pVarT] = auto_increment._llMaxValue;
    }

    return true;
}

void CEntryDriver::InitializeAutoIncrementFields()
{
    if( !LoadAutoIncrementFields() )
        return;

    for( auto itr = m_pmapAutoIncrementFields->begin(); itr != m_pmapAutoIncrementFields->end(); itr++ )
    {
        int64_t llIncrementedValue = itr->second + 1;

        VART* pVarT = itr->first;
        VARX* pVarX = pVarT->GetVarX();

        TCHAR* pszVarText = m_pIntDriver->GetVarAsciiAddr(pVarT);
        double* pFlotAddr = m_pIntDriver->GetVarFloatAddr(pVarX);

        *pFlotAddr = (double)llIncrementedValue;
        pVarT->dvaltochar(*pFlotAddr,pszVarText);
    }
}

void CEntryDriver::UpdateAutoIncrementField(VART* pVarT)
{
    if( m_pmapAutoIncrementFields == nullptr ) // this will happen if nothing has been opened in add mode
        return;

    double* pFlotAddr = m_pIntDriver->GetVarFloatAddr(pVarT->GetVarX());
    int64_t llThisValue = (int64_t)*pFlotAddr;

    if( llThisValue > m_pmapAutoIncrementFields->at(pVarT) )
        m_pmapAutoIncrementFields->at(pVarT) = llThisValue;
}


void CEntryDriver::PrefillKeyFromPff()
{
    CString key = m_pPifFile->GetKey();
    int current_position_in_key = 0;

    for( int i = 0; current_position_in_key < key.GetLength() && QidVars[0][i] >= 0; i++ )
    {
        VART* pVarT = VPT(QidVars[0][i]);
        TCHAR* variable_text_buffer = m_pIntDriver->GetVarAsciiAddr(pVarT);

        // format the value properly for this variable
        CIMSAString value = key.Mid(current_position_in_key, pVarT->GetLength());
        value.MakeExactLength(pVarT->GetLength());
        current_position_in_key += pVarT->GetLength();

        _tmemcpy(variable_text_buffer, value, value.GetLength());

        if( pVarT->IsNumeric() )
        {
            ASSERT(pVarT->GetDecimals() == 0);
            double* variable_double_buffer = m_pIntDriver->GetVarFloatAddr(pVarT);
            *variable_double_buffer = chartodval(value, pVarT->GetLength(), 0);

            // convert from MASKBLK to NOTAPPL because if the field is protected,
            // an incorrect value will be left in the buffers
            if( *variable_double_buffer == MASKBLK )
                *variable_double_buffer = NOTAPPL;
        }
    }

    // if the whole key was used, turn AutoAdd off to avoid duplicate keys
    if( current_position_in_key >= DIP(0)->qlen[0] )
        m_pPifFile->SetAutoAddFlag(false);
}


void CEntryDriver::PrefillNonPersistentFields()
{
    // load and setup the values
    if( m_prefilledNonPersistentFields == nullptr )
    {
        m_prefilledNonPersistentFields = std::make_unique<std::map<VART*, std::wstring>>();

        for( const auto& [field_name, field_value] : m_pPifFile->GetCustomParams() )
        {
            // check if this is a valid field on the first level, not persistent, and not repeating
            Symbol* symbol = DIP(0)->FindChildSymbol(field_name);

            if( symbol != nullptr && symbol->IsA(SymbolType::Variable) )
            {
                VART* pVarT = assert_cast<VART*>(symbol);

                if( pVarT->GetLevel() == 1 && !pVarT->IsPersistent() && !pVarT->IsArray() )
                {
                    // format the value properly for this variable
                    std::wstring formatted_value;

                    if( pVarT->IsAlpha() )
                    {
                        formatted_value = field_value;
                        SO::MakeExactLength(formatted_value, pVarT->GetLength());
                    }

                    else
                    {
                        double numeric_value = CIMSAString::fVal(field_value);
                        formatted_value = pVarT->GetCurrentValueProcessor().GetOutput(numeric_value);
                    }

                    m_prefilledNonPersistentFields->try_emplace(pVarT, formatted_value);
                }
            }
        }
    }

    // fill the values
    if( m_prefilledNonPersistentFields != nullptr && !m_prefilledNonPersistentFields->empty() )
    {
        for( const auto& [pVarT, formatted_value] : *m_prefilledNonPersistentFields )
        {
            TCHAR* variable_text_buffer = m_pIntDriver->GetVarAsciiAddr(pVarT);
            _tmemcpy(variable_text_buffer, formatted_value.c_str(), formatted_value.length());

            if( pVarT->IsNumeric() )
            {
                double* variable_double_buffer = m_pIntDriver->GetVarFloatAddr(pVarT);
                *variable_double_buffer = pVarT->GetCurrentValueProcessor().GetNumericFromInput(WS2CS(formatted_value));
            }
        }
    }
}


ResponseProcessor* CEntryDriver::GetResponseProcessor(const DEFLD* defld)
{
    VART* pVarT = VPT(defld->GetSymbol());
    CaptureType evaluated_capture_type = pVarT->GetEvaluatedCaptureInfo().GetCaptureType();

    if( evaluated_capture_type != CaptureType::RadioButton  && evaluated_capture_type != CaptureType::CheckBox &&
        evaluated_capture_type != CaptureType::DropDown     && evaluated_capture_type != CaptureType::ComboBox &&
        evaluated_capture_type != CaptureType::ToggleButton )
    {
        return nullptr;
    }

    ResponseProcessor* response_processor = pVarT->GetCurrentValueSet()->GetResponseProcessor();

    // modify the response processor to respond to any dynamic settings
    response_processor->ApplyCaptureTypeProperties(evaluated_capture_type);

    if( pVarT->IsNumeric() )
    {
        response_processor->SetCanEnterNotAppl(pVarT->AddNotApplToValueSet());

        // potentially remove the range responses if only showing discrete values
        if( evaluated_capture_type == CaptureType::ComboBox )
            response_processor->RemoveRangeResponses(m_pEntryDriver->GetApplication()->GetComboBoxShowOnlyDiscreteValues());

        // show refused codes if always showing responses or if the value is currently refused
        if( response_processor->ResponsesIncludeRefused() )
        {
            bool show_refusals = m_pEntryDriver->GetApplication()->GetShowRefusals() ||
                                 ( m_pIntDriver->GetVarFloatValue(pVarT, defld->GetIndexes()) == REFUSED );
            response_processor->ShowRefusedValue(show_refusals);
        }
    }

    return response_processor;
}


CString CEntryDriver::GetNoteContent(const DEFLD& defld)
{
    std::shared_ptr<NamedReference> named_reference;
    int field_symbol;
    GetNamedReferenceFromField(defld, named_reference, field_symbol);

    return CEngineDriver::GetNoteContent(named_reference, GetOperatorId(), field_symbol);
}


void CEntryDriver::SetNote(const DEFLD& defld, const CString& note_content)
{
    std::shared_ptr<NamedReference> named_reference;
    int field_symbol;
    GetNamedReferenceFromField(defld, named_reference, field_symbol);

    CEngineDriver::SetNote(named_reference, GetOperatorId(), note_content, field_symbol);
}


// for on-demand (interface) editing of notes
bool CEntryDriver::EditNote(bool case_note, const DEFLD* defld/* = nullptr*/)
{
    std::shared_ptr<NamedReference> named_reference;
    int field_symbol;

    if( case_note )
    {
        const DICT* pDicT = DIP(0);
        named_reference = std::make_shared<NamedReference>(WS2CS(pDicT->GetName()), CString());
        field_symbol = pDicT->GetSymbolIndex();
    }

    else
    {
        // if not passing a specific field, use the current one
        if( defld == nullptr )
        {
            defld = GetCsDriver()->GetCurDeFld();

            if( defld == nullptr )
                return false;
        }

        GetNamedReferenceFromField(*defld, named_reference, field_symbol);
    }

    return std::get<bool>(CEngineDriver::EditNote(named_reference, case_note ? CString() : GetOperatorId(), field_symbol));
}


int CEntryDriver::DisplayMessage(MessageType message_type, int message_number, const std::wstring& message_text, const void* extra_information/* = nullptr*/)
{
    // abort messages aren't displayed using the entry message because they are handled elsewhere
    ASSERT(message_type != MessageType::Abort);

    if( !UseHtmlDialogs() )
        return DisplayMessage_pre77(message_type, message_number, WS2CS(message_text), extra_information);

    cs::shared_or_raw_ptr<const std::vector<std::wstring>> message_buttons;
    int default_button_index = 0;

    if( extra_information != nullptr )
    {
        const std::tuple<std::vector<std::wstring>, int>& entry_error_information = *static_cast<const std::tuple<std::vector<std::wstring>, int>*>(extra_information);
        message_buttons = &std::get<0>(entry_error_information);
        default_button_index = std::get<1>(entry_error_information);

        // the default button index needs to be one-based
        ++default_button_index;
        ASSERT(static_cast<size_t>(default_button_index) <= message_buttons->size());
    }

    // when not using select, the only button is the OK button
    if( message_buttons == nullptr || message_buttons->empty() )
    {
#ifdef WIN_DESKTOP
        // use the old message style while in operator-controlled mode
        if( WindowsDesktopMessage::Send(UWM::CSEntry::UsingOperatorControlledMessages) == 1 )
            return DisplayMessage_pre77(message_type, message_number, WS2CS(message_text), extra_information);

        // CSEntry may decide that this error message does not need to be displayed
        // (for example, while in an interactive edit)
        if( WindowsDesktopMessage::Send(UWM::CSEntry::PreprocessEngineMessage, static_cast<WPARAM>(message_type), message_number) < 0 )
            return 1;
#endif

        // to allow simple error messages to display during synchronous JavaScript calls
        // into the engine, display messages using a native message box
        if( WebViewSyncOperationMarker::IsOperationInProgress() )
        {
            ErrorMessage::Display(message_text);
            return 1;
        }

        // add the default OK text
        message_buttons = std::make_shared<std::vector<std::wstring>>(std::vector<std::wstring>({ MGF::GetMessageText(MGF::Ok) }));
    }

    ErrmsgDlg errmsg_dlg;

    // construct the title
#ifndef WIN_DESKTOP
    if( GetApplication()->GetShowErrorMessageNumbers() )
#endif
    {
        const std::wstring& message_type_text =
            ( message_type == MessageType::User )    ? MGF::GetMessageText(MGF::UserErrorTitle) :
            ( message_type == MessageType::Warning ) ? MGF::GetMessageText(MGF::SystemWarningTitle) :
                                                       MGF::GetMessageText(MGF::SystemErrorTitle);

        errmsg_dlg.SetTitle(FormatTextCS2WS(_T("%s (%d)"), message_type_text.c_str(), message_number));
    }

    // add the message text, default button index, and buttons
    errmsg_dlg.SetMessage(message_text);
    errmsg_dlg.SetDefaultButtonIndex(default_button_index);
    errmsg_dlg.SetButtons(*message_buttons);

#ifdef WIN_DESKTOP
    ::MessageBeep(0);
#endif

    while( true )
    {
        if( errmsg_dlg.DoModalOnUIThread() == IDOK )
        {
            // only return once the user has made a valid selection
            if( errmsg_dlg.GetSelectedButtonIndex() != 0 )
                return errmsg_dlg.GetSelectedButtonIndex();
        }
    }
}


int CEntryDriver::DisplayMessage_pre77(MessageType message_type, int message_number, const CString& message_text, const void* extra_information/* = nullptr*/)
{
    std::vector<CString> message_buttons;
    int default_button_number = -1;

    if( extra_information != nullptr )
    {
        const std::tuple<std::vector<std::wstring>, int>& entry_error_information = *static_cast<const std::tuple<std::vector<std::wstring>, int>*>(extra_information);
        message_buttons = WS2CS_Vector(std::get<0>(entry_error_information));
        default_button_number = std::get<1>(entry_error_information);
    }

    CString message = message_text;
    message.TrimRight();

    // construct the title
    CString title;

#ifndef WIN_DESKTOP
    if( GetApplication()->GetShowErrorMessageNumbers() )
#endif
    {
        const std::wstring& message_type_text =
            ( message_type == MessageType::User )    ? MGF::GetMessageText(MGF::UserErrorTitle) :
            ( message_type == MessageType::Warning ) ? MGF::GetMessageText(MGF::SystemWarningTitle) :
                                                       MGF::GetMessageText(MGF::SystemErrorTitle);

        title.Format(_T("%s (%d)"), message_type_text.c_str(), message_number);
    }

    CMsgOptions message_options(title, message, MB_OK, default_button_number, -1, message_buttons, message_type, message_number);

    while( true )
    {
#ifdef WIN_DESKTOP
        const int selected_button_number = AfxGetMainWnd()->SendMessage(WM_IMSA_ENGINEMSG, (WPARAM)&message_options);
#else
        const int selected_button_number = PlatformInterface::GetInstance()->GetApplicationInterface()->ShowMessage(title, message, message_buttons);
#endif

        // only return once the user has made a valid selection and then return the button number (one-based)
        if( selected_button_number >= 0 )
            return selected_button_number + 1;
    }
}


void CEntryDriver::ViewCurrentCase()
{
    // determine what dictionary is currently being entered
    const DEFLD* defld = GetCsDriver()->GetCurDeFld();

    if( defld == nullptr )
    {
        ASSERT(false);
        return;
    }

    const VART* pVarT = VPT(defld->GetSymbol());
    const DICT* pDicT = pVarT->GetDPT();
    ASSERT(pDicT != nullptr);

    // if the user has specified an OnViewQuestionnaire function, it should be executed
    // with the return value indicating whether to show the default case view
    if( m_pIntDriver->HasSpecialFunction(SpecialFunction::OnViewQuestionnaire) )
    {
        const double return_value = m_pIntDriver->ExecSpecialFunction(pVarT->GetSymbolIndex(),
                                                                      SpecialFunction::OnViewQuestionnaire,
                                                                      { pDicT->GetName() });

        // return if the user wants to suppress showing the current case
        if( CIntDriver::ConditionalValueIsFalse(return_value) )
            return;
    }

    // because the Action Invoker will be called to get the case data, set the proper attributes to allow those calls to succeed
    const RAII::SetValueAndRestoreOnDestruction prog_type_modifier(m_pIntDriver->m_iProgType, PROCTYPE_ONFOCUS);
    const RAII::SetValueAndRestoreOnDestruction symbol_modifier(m_pIntDriver->m_iExSymbol, pVarT->GetSymbolIndex());
    const RAII::SetValueAndRestoreOnDestruction level_modifier(m_pIntDriver->m_iExLevel, SymbolCalculator::GetLevelNumber_base1(*pVarT));

    EngineQuestionnaireViewer engine_questionnaire_viewer(this, pDicT->GetName());
    engine_questionnaire_viewer.View();
}


void CEntryDriver::ViewCase(const double position_in_repository)
{
    try
    {
        DataRepository* input_data_repository = GetInputRepository();
        std::unique_ptr<Case> data_case = input_data_repository->GetCaseAccess()->CreateCase();

        input_data_repository->ReadCase(*data_case, position_in_repository);

        EngineQuestionnaireViewer engine_questionnaire_viewer(this, CS2WS(input_data_repository->GetCaseAccess()->GetDataDict().GetName()),
                                                              CS2WS(data_case->GetUuid()), CS2WS(data_case->GetKey()));

        engine_questionnaire_viewer.View();
    }

    catch( const CSProException& exception )
    {
        ASSERT(false);
        ErrorMessage::Display(exception);
    }
}
