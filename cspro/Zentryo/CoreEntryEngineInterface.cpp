#include "StdAfx.h"
#include <zPlatformO/PlatformInterface.h>
#include "CoreEntryEngineInterface.h"
#include "CoreEntryPage.h"
#include "CoreEntryPageField.h"
#include "CaseTreeBuilder.h"
#include <zPlatformO/PortableMFC.h>
#include <zToolsO/Serializer.h>
#include <zToolsO/Tools.h>
#include <zToolsO/Utf8Convert.h>
#include <zUtilO/AppLdr.h>
#include <zUtilO/CommonStore.h>
#include <zUtilO/ExecutionStack.h>
#include <zUtilO/imsaStr.h>
#include <zUtilO/MemoryHelpers.h>
#include <zAppO/Application.h>
#include <zCaseO/CaseItemReference.h>
#include <zCaseO/NumericCaseItem.h>
#include <zDataO/CaseAccessSaver.h>
#include <zDataO/DataRepository.h>
#include <zDictO/DDClass.h>
#include <ZBRIDGEO/npff.h>
#include <zMessageO/Messages.h>
#include <engine/EngineObjectTransporter.h>
#include <engine/IntDrive.h>
#include <engine/ParadataDriver.h>
#include <regex>


CoreEntryEngineInterface::CoreEntryEngineInterface()
    :   m_pRunAplEntry(nullptr),
        m_pPifFile(nullptr),
        m_currentEntryPage(nullptr),
        m_bCaseModified(false),
        m_caseTreeBuilder(nullptr),
        m_pEngineDriver(nullptr),
        m_engineData(nullptr),
        m_pIntDriver(nullptr)
{
}


CoreEntryEngineInterface::~CoreEntryEngineInterface()
{
    Cleanup();
}


void CoreEntryEngineInterface::Cleanup()
{
    m_objectTransporter.reset();

    safe_delete(m_currentEntryPage);

    m_caseTree.reset();
    safe_delete(m_caseTreeBuilder);

    if( m_pRunAplEntry != nullptr )
    {
        m_pRunAplEntry->End(FALSE);
        safe_delete(m_pRunAplEntry);
    }

    safe_delete(m_pPifFile);

    m_pEngineDriver = nullptr;
    m_engineData = nullptr;
    m_pIntDriver = nullptr;
}


ObjectTransporter* CoreEntryEngineInterface::GetObjectTransporter()
{
    // create a new Object Transporter every time the engine driver changes
    if( !m_objectTransporter.has_value() || std::get<0>(*m_objectTransporter) != m_pEngineDriver )
    {
        const CEngineArea* engine_area = ( m_pEngineDriver != nullptr ) ? &m_pEngineDriver->m_EngineArea :
                                                                          nullptr;
        std::unique_ptr<ObjectTransporter> object_transporter_override;

        // if there is no engine driver, it means this is coming from ActionInvokerActivity or CSView
        if( engine_area == nullptr )
        {
            ASSERT(ExecutionStack::GetEntries().size() == 1);
            const ExecutionStack::EntryType& execution_stack_entry_type = ExecutionStack::GetEntries().begin()->second;

            // if CSView, disable the need for Action Invoker access tokens
            if( !std::holds_alternative<ExecutionStack::ActionInvokerActivity>(execution_stack_entry_type) )
            {
                ASSERT(std::get<cs::non_null_shared_or_raw_ptr<const PFF>>(execution_stack_entry_type)->GetAppType() == APPTYPE::VIEW_TYPE);

                class CSViewObjectTransporter : public CommonObjectTransporter
                {
                    bool DisableAccessTokenCheckForExternalCallers() const override { return true; }
                };

                object_transporter_override = std::make_unique<CSViewObjectTransporter>();
            }
        }

        m_objectTransporter.emplace(m_pEngineDriver, ( object_transporter_override != nullptr ) ? std::move(object_transporter_override) :
                                                                                                  std::make_unique<EngineObjectTransporter>(engine_area));
    }

    return std::get<1>(*m_objectTransporter).get();
}


const Logic::SymbolTable& CoreEntryEngineInterface::GetSymbolTable() const
{
    return m_engineData->symbol_table;
}


bool CoreEntryEngineInterface::InitApplication(const CString& pff_filename)
{
    CString workingFolder = pff_filename;
    PathRemoveFileSpec(workingFolder.GetBuffer());
    workingFolder.ReleaseBuffer();
    PlatformInterface::GetInstance()->SetWorkingDirectory(CS2WS(workingFolder));

    m_pPifFile = new CNPifFile(pff_filename);
    m_pPifFile->LoadPifFile();
    m_pPifFile->SetBinaryLoad(true);
    m_pPifFile->SetAutoAddFlag(false); // force auto add off - we are adding only one case per start

    if( !m_pPifFile->BuildAllObjects() )
        return false;

    Application* pApplication = m_pPifFile->GetApplication();
    m_pRunAplEntry = new CRunAplEntry(m_pPifFile);

    pApplication->SetCompiled(false);

    try
    {
        m_pRunAplEntry->LoadCompile();
    }

    catch(...)
    {
        return false;
    }

    APP_LOAD_TODO_GetArchive().CloseArchive();

    pApplication->SetCompiled(true);

    if( !m_pRunAplEntry->FinalizeInitializationTasks() )
        return false;

    for( const auto& form_file : pApplication->GetRuntimeFormFiles() )
        form_file->RefreshAssociatedFieldText();

    m_pEngineDriver = m_pRunAplEntry->GetEntryDriver();
    m_engineData = &m_pEngineDriver->GetEngineData();
    m_pIntDriver = m_pEngineDriver->m_pIntDriver.get();

    m_caseTreeBuilder = new CaseTreeBuilder(m_pRunAplEntry);

    // if a specific common store is specified, open it so that any system settings come from that file
    if( !m_pPifFile->GetCommonStoreFName().IsEmpty() )
        GetCommonStore();

    return true;
}


CoreEntryPage* CoreEntryEngineInterface::ProcessFieldPostMovement(CDEField* pField)
{
    safe_delete(m_currentEntryPage);

    if( pField != nullptr )
    {
        // if the field is on a block that should be displayed together, reenter
        // the first (not skipped over) field on the block, or in operator-controlled
        // mode, the first valid field
        const VART* field_vart = VPT(pField->GetSymbol());
        const EngineBlock* engine_block = field_vart->GetEngineBlock();

        if( engine_block != nullptr && engine_block->GetFormBlock().GetDisplayTogether() )
        {
            CDEField* first_field_in_block = nullptr;
            bool ignore_field_status = !m_pRunAplEntry->IsPathOn();

            for( CDEField* this_field : engine_block->GetFields() )
            {
                bool enter_this_field = ( this_field == pField );

                if( !enter_this_field )
                {
                    if( !this_field->IsProtected() && !this_field->IsMirror() )
                    {
                        enter_this_field = ignore_field_status ||
                            ( m_pRunAplEntry->GetStatus(this_field->GetSymbol(), pField->GetRuntimeOccurrence()) == FLAG_HIGHLIGHT );
                    }
                }

                if( enter_this_field )
                {
                    first_field_in_block = this_field;
                    break;
                }
            }

            ASSERT(first_field_in_block != nullptr);

            // if not already on the first field, then we will reenter it
            if( first_field_in_block != pField )
            {
                first_field_in_block =
                    (CDEField*)m_pRunAplEntry->MoveToField(first_field_in_block->GetSymbol(), pField->GetRuntimeOccurrence(), true);

                return ProcessFieldPostMovement(first_field_in_block);
            }
        }

        // create the entry page
        m_currentEntryPage = new CoreEntryPage(this, pField);
    }

    return m_currentEntryPage;
}


CoreEntryPage* CoreEntryEngineInterface::EndGroup()
{
    for( const CoreEntryPageField& page_field : m_currentEntryPage->GetPageFields() )
    {
        // don't endgroup if the field is an ID item
        if( page_field.GetField()->GetDictItem()->GetRecord()->GetSonNumber() == COMMON )
            return m_currentEntryPage;
    }

    // only run the postproc if the field isn't empty
    bool bRunPostProc = m_currentEntryPage->AreAnyFieldsFilled();

    // in add mode on rosters with sequential fields, check that a non-sequential value
    // is filled in on the current occurrence; if not, delete the occurrence
    CDEGroup* pParentGroup = m_currentEntryPage->GetField()->GetParent();

    if( m_pRunAplEntry->InAddMode() && pParentGroup->GetMaxLoopOccs() > 1 )
    {
        int iCurOccurrence = pParentGroup->GetCurOccurrence();
        bool bOccurrenceHasData = false;

        for( int i = 0; !bOccurrenceHasData && i < pParentGroup->GetNumItems(); i++ )
        {
            CDEField* pRosterField = (CDEField*)pParentGroup->GetItem(i);

            if( !pRosterField->IsSequential() )
            {
                const CDictItem* pRosterItem = pRosterField->GetDictItem();

                CString csValue = m_pRunAplEntry->GetVal(pRosterItem->GetSymbol(), iCurOccurrence);

                for( int j = 0; !bOccurrenceHasData && j < csValue.GetLength(); j++ )
                    bOccurrenceHasData = ( csValue[j] != _T(' ') );
            }
        }

        if( !bOccurrenceHasData )
        {
            DeleteOcc();
            bRunPostProc = false;
        }
    }

    CDEField* pField = (CDEField*)m_pRunAplEntry->EndGroup(bRunPostProc);

    return ProcessFieldPostMovement(pField);
}


CoreEntryPage* CoreEntryEngineInterface::EndLevel()
{
    // get the number of levels for the current flow
    int iNumLevels = m_pRunAplEntry->GetNumLevels(false);

    // for one level apps, call EndLevelOcc to end the level
    if( iNumLevels == 1 )
        return EndLevelOcc();

    // if on the first level of a multiple level app, don't endlevel
    if( m_pRunAplEntry->GetCurrentLevel() == 1 )
        return m_currentEntryPage;

    // only run the postproc if the field isn't empty
    bool bRunPostProc = m_currentEntryPage->AreAnyFieldsFilled();

    CDEField* pField = (CDEField*)m_pRunAplEntry->EndLevel(bRunPostProc, false, m_pRunAplEntry->GetCurrentLevel() - 1, true);

    return ProcessFieldPostMovement(pField);
}


CoreEntryPage* CoreEntryEngineInterface::EndLevelOcc()
{
    // only run the postproc if the field isn't empty
    bool bRunPostProc = m_currentEntryPage->AreAnyFieldsFilled();

    CDEField* pField = m_pRunAplEntry->IsPathOn() ?
        (CDEField*)m_pRunAplEntry->AdvanceToEnd(false, false) :
        (CDEField*)m_pRunAplEntry->EndLevel(bRunPostProc, false, m_pRunAplEntry->GetCurrentLevel(), true);

    return ProcessFieldPostMovement(pField);
}


CoreEntryPage* CoreEntryEngineInterface::AdvanceToEnd()
{
    CDEField* pField = (CDEField*)m_pRunAplEntry->AdvanceToEnd(false, FALSE);
    return ProcessFieldPostMovement(pField);
}


CoreEntryPage* CoreEntryEngineInterface::NextField()
{
    bool advance_past_block = ( m_currentEntryPage->GetPageFields().size() > 1 );
    CDEField* pField = (CDEField*)m_pRunAplEntry->NextField(FALSE, advance_past_block);
    return ProcessFieldPostMovement(pField);
}


CoreEntryPage* CoreEntryEngineInterface::PreviousField()
{
    CDEField* pField = (CDEField*)m_pRunAplEntry->PreviousField(FALSE);
    return ProcessFieldPostMovement(pField);
}


CoreEntryPage* CoreEntryEngineInterface::PreviousPersistentField()
{
    CDEField* pField = (CDEField*)m_pRunAplEntry->PreviousPersistentField();
    return ProcessFieldPostMovement(pField);
}


CoreEntryPage* CoreEntryEngineInterface::GoToField(int fieldSymbol, const int index[3])
{
    CNDIndexes theIndex(ONE_BASED, index);
    DEFLD cField(fieldSymbol, theIndex);

    CDEField* pField = (CDEField*)m_pRunAplEntry->MoveToField(&cField, true);

    return ProcessFieldPostMovement(pField);
}


bool CoreEntryEngineInterface::ProcessOccurrenceModification(CDEItemBase* (CRunAplEntry::*pOccurrenceModificationFunction)(bool& bRet))
{
    bool bRet = false;

    CDEField* pField = (CDEField*)(m_pRunAplEntry->*pOccurrenceModificationFunction)(bRet);
    ProcessFieldPostMovement(pField);

    m_bCaseModified |= bRet;

    return bRet;
}


bool CoreEntryEngineInterface::InsertOcc()
{
    return ProcessOccurrenceModification(&CRunAplEntry::InsertOcc);
}


bool CoreEntryEngineInterface::InsertOccAfter()
{
    return ProcessOccurrenceModification(&CRunAplEntry::InsertOccAfter);
}


bool CoreEntryEngineInterface::DeleteOcc()
{
    return ProcessOccurrenceModification(&CRunAplEntry::DeleteOcc);
}


bool CoreEntryEngineInterface::IsSystemControlled() const
{
    return m_pRunAplEntry->IsPathOn();
}


bool CoreEntryEngineInterface::ContainsMultipleLanguages() const
{
    return ( m_pRunAplEntry->GetLanguages(false).size() > 1 );
}


#ifdef BLOCK_TODO
//Used for getting the field value for display only. For updates you need to call procesdecimalvalue to set the zerofill etc correctly
void CoreEntryEngineInterface::GetFieldValue(CDEField* pField)
{
    if (m_pRunAplEntry != NULL && pField) {
        CIMSAString csData;
        CDictItem* pItem = pField->GetDictItem();

        TCHAR* pszBuf = new TCHAR[pItem->GetLen() + 1];
        _tmemset(pszBuf, 0, pItem->GetLen() + 1);

        if (m_pRunAplEntry->GetVal(pItem->GetSymbol(), 0, pszBuf, nullptr)) {
            csData = pszBuf;

            csData = ProcessDecimalValue(pField->GetDictItem(), csData, false); //Process decimal for casetree display purpose.

            //for display purposes do not show zero fill.
            if (pItem->GetContentType() == ContentType::Numeric && csData.GetLength() > 1 && csData.Find(_T("0")) != -1) {
                if (csData.Find(_T("-")) != -1) {
                    //remove leading blanks, leading negative sign and leading zeroes
                    csData = csData.TrimLeft(_T(" -0"));
                   //add the negative symbol back again
                    csData = _T("-") + csData;
                } else {
                    csData = csData.TrimLeft(L"0");
                }
                if (csData.IsEmpty()) {//if the string is blank make sure it shows 0
                    csData = _T("0");
                } else if (csData[0] == _T('.')) {//if the leading character is a . make sure that it shows 0.1223
                    csData = _T("0") + csData;
                }
                if (pItem->GetDecimal() > 1 && csData.Find(_T(".")) != -1) {//Decimal present
                    csData = csData.TrimRight(_T('0')); //Right trim 0
                    if (csData[csData.GetLength() - 1] == _T('.')) { //make sure a . is not he last character
                        csData = csData + _T("0");
                    }

                }
            }
            if (m_pRunAplEntry->InAddMode() && pField->IsSequential()) {
                //in add mode for sequential field. Get the value from the object and not from the buffers.
                //buffers are not updated until the field enter is pressed on the uo
                csData = pField->GetData();
            } else {
                pField->SetData(csData);
            }
        }

        delete[] pszBuf;
    }
}
#endif


bool CoreEntryEngineInterface::Start(std::optional<double> insert_before_position_in_repository/* = std::nullopt*/)
{
    bool result = false;

    if (m_pRunAplEntry != NULL)
    {
        result = (bool)m_pRunAplEntry->Start(CRUNAPL_ADD);

        if( result )
        {
            if( insert_before_position_in_repository.has_value() )
                m_pRunAplEntry->ProcessInsert(*insert_before_position_in_repository);

            else
                m_pRunAplEntry->ProcessAdd();

            SetCaseModified(false);

            // set the current field to the first one
            CDEField* pField = (CDEField*)m_pRunAplEntry->NextField(FALSE);

            ProcessFieldPostMovement(pField);
        }
    }

    return result;
}


//TODO: For multiple levels node will not be 0. The function args should change to pass node.
bool CoreEntryEngineInterface::ModifyCase(double position_in_repository)
{
    if (!m_pRunAplEntry->HasStarted()) {
        m_pRunAplEntry->Start(CRUNAPL_MODIFY);
    }

    if( !m_pRunAplEntry->ModifyStart() ) {
        ShowModalDialog(MGF::GetMessageText(MGF::SystemErrorTitle), MGF::GetMessageText(MGF::ErrorStartModify), MB_OK);
        return false;
    }

    SetCaseModified(false);

    bool bMoved = false;
    PartialSaveMode partial_save_mode = PartialSaveMode::None;
    CDEItemBase* pItem = NULL;

    if( !m_pRunAplEntry->ProcessModify(position_in_repository, &bMoved, &partial_save_mode, &pItem) )
    {
        m_pRunAplEntry->ModifyStop();
        m_pRunAplEntry->Stop();
        return false;
    }

    ProcessFieldPostMovement((CDEField*)pItem);

    return true;
}


bool CoreEntryEngineInterface::InsertCase(double insert_before_position_in_repository)
{
    m_pRunAplEntry->Start(CRUNAPL_ADD);
    m_pRunAplEntry->Stop();

    return Start(insert_before_position_in_repository);
}


class FirstLevelSingleNumericItemValueGetter
{
    int m_record_index;
    const NumericCaseItem* m_case_item;

public:
    FirstLevelSingleNumericItemValueGetter(const CaseAccess& case_access, const CDictItem& item)
    {
        const auto& case_item = case_access.LookupCaseItem(item);
        // make sure that this is valid dictionary name, and numeric
        if (case_item == nullptr || !case_item->IsTypeNumeric())
            throw std::invalid_argument("Not a valid item");

        m_case_item = static_cast<const NumericCaseItem*>(case_item);

        const auto& dict_record = case_item->GetDictionaryItem().GetRecord();

        // make sure that it is on the root level
        if( dict_record->GetLevel()->GetLevelNumber() != 0 )
            throw std::invalid_argument("Not a valid item");

        m_record_index = case_item->GetDictionaryItem().GetRecord()->GetSonNumber();
    }

    double Get(const Case& data_case) const
    {
        const auto& case_record = data_case.GetRootCaseLevel().GetCaseRecord(static_cast<size_t>(m_record_index));
        if (!case_record.HasOccurrences())
            throw std::out_of_range("No record occurrence");
        auto index = case_record.GetCaseItemIndex();
        return m_case_item->GetValue(index);
    }
};


class LatLonGetter
{
    FirstLevelSingleNumericItemValueGetter m_lat_retriever;
    FirstLevelSingleNumericItemValueGetter m_lon_retriever;

public:
    LatLonGetter(const CaseAccess& case_access, const CDictItem& latitude_item_name, const CDictItem& longitude_item_name):
        m_lat_retriever(case_access, latitude_item_name),
        m_lon_retriever(case_access, longitude_item_name)
    {
    }

    double GetLatitude(const Case& data_case) const
    {
        return m_lat_retriever.Get(data_case);
    }

    double GetLongitude(const Case& data_case) const
    {
        return m_lon_retriever.Get(data_case);
    }
};


std::vector<CoreEntryEngineInterface::CaseSummaryWithLatLong>
CoreEntryEngineInterface::GetSequentialCaseIds(bool sort_ascending,
                                               const std::optional<CString>& lat_item_name,
                                               const std::optional<CString>& lon_item_name)
{
    std::vector<CaseSummaryWithLatLong> case_summaries;

    std::function<bool(const CaseSummary&)> key_filter = [](const CaseSummary &caseSummary) { return true; };
    const CString csFilterRegex = m_pPifFile->GetCaseListingFilter();
    std::unique_ptr<std::regex> caseFilterRegex;
    if (!csFilterRegex.IsEmpty()) {
        try {
            caseFilterRegex = std::unique_ptr<std::regex>(new std::regex(UTF8Convert::WideToUTF8(csFilterRegex)));
            key_filter = [&caseFilterRegex](const CaseSummary &case_summary) { return std::regex_search(UTF8Convert::WideToUTF8(case_summary.GetKey()), *caseFilterRegex); };
        }
        catch (const std::regex_error&) {
            ShowModalDialog(_T(""), FormatText(_T("CaseListingFilter is an invalid ECMAScript regular expression: %s"), csFilterRegex.GetString()), MB_OK);
        }
    }

    try
    {
        DataRepository* pInputRepo = m_pRunAplEntry->GetInputRepository();
        CaseAccessSaver case_access_saver(*pInputRepo);

        std::optional<LatLonGetter> lat_lon_getter;
        if (lat_item_name && lon_item_name) {
            auto case_item_case_access = std::make_shared<CaseAccess>(pInputRepo->GetCaseAccess()->GetDataDict());
            case_item_case_access->SetUsesAllCaseAttributes();
            auto lat_dict_item = case_item_case_access->GetDataDict().FindItem(*lat_item_name);
            auto lon_dict_item = case_item_case_access->GetDataDict().FindItem(*lon_item_name);
            if (lat_dict_item && lon_dict_item) {
                case_item_case_access->SetUseDictionaryItem(*lat_dict_item);
                case_item_case_access->SetUseDictionaryItem(*lon_dict_item);
                case_item_case_access->Initialize();
                pInputRepo->ModifyCaseAccess(case_item_case_access);
                try {
                    lat_lon_getter = LatLonGetter(*pInputRepo->GetCaseAccess(),
                                                  *lat_dict_item,
                                                  *lon_dict_item);
                } catch (std::exception &) {
                    // if there is an error just use lat/long default vals of zero
                }
            }
        }

        auto case_summary_iterator = pInputRepo->CreateIterator(lat_lon_getter ? CaseIterationContent::Case : CaseIterationContent::CaseSummary,
            CaseIterationCaseStatus::NotDeletedOnly, CaseIterationMethod::SequentialOrder, sort_ascending ? CaseIterationOrder::Ascending : CaseIterationOrder::Descending);

        if (lat_lon_getter) {
            Case data_case(pInputRepo->GetCaseAccess()->GetCaseMetadata());
            while (case_summary_iterator->NextCase(data_case)) {
                if (key_filter(data_case)) {
                    double latitude = 0;
                    double longitude = 0;
                    try {
                        latitude = lat_lon_getter->GetLatitude(data_case);
                        longitude = lat_lon_getter->GetLongitude(data_case);
                    } catch (const std::exception&) {
                        // if there is an error just use lat/long default vals of zero
                    }

                    if( IsSpecial(latitude) || IsSpecial(longitude) ) {
                        latitude = 0;
                        longitude = 0;
                    }

                    case_summaries.emplace_back(CaseSummaryWithLatLong{data_case, latitude, longitude});
                }
            }
        } else {
            CaseSummary case_summary;
            while (case_summary_iterator->NextCaseSummary(case_summary))
            {
                if (key_filter(case_summary))
                    case_summaries.emplace_back(CaseSummaryWithLatLong{case_summary, 0, 0});
            }
        }
    }

    catch( const DataRepositoryException::Error& exception )
    {
        ShowModalDialog(_T(""), exception.GetErrorMessage(), MB_OK);
    }

    return case_summaries;
}


bool CoreEntryEngineInterface::DeleteCase(double position_in_repository)
{
    try
    {
        // delete the case
        DataRepository* pInputRepo = m_pRunAplEntry->GetInputRepository();
        pInputRepo->DeleteCase(position_in_repository);
        return true;
    }

    catch( const DataRepositoryException::Error& exception )
    {
        ShowModalDialog(_T(""), exception.GetErrorMessage(), MB_OK);
    }

    return false;
}


bool CoreEntryEngineInterface::GetAskOpIDFlag() const
{
    return m_pPifFile->GetApplication()->GetAskOperatorId();
}


void CoreEntryEngineInterface::SetOperatorId(CString operatorID)
{
    m_pRunAplEntry->SetOperatorId(operatorID);
}


int CoreEntryEngineInterface::GetStopCode() const
{
    return m_pRunAplEntry->GetEntryDriver()->Exit_Code;
}


void CoreEntryEngineInterface::OnStop()
{
    if( m_pRunAplEntry == nullptr )
        return;

    m_pRunAplEntry->ResetDoorCondition();

    if( m_pRunAplEntry->HasModifyStarted() )
        m_pRunAplEntry->ModifyStop();

    m_pRunAplEntry->Stop();

    safe_delete(m_currentEntryPage);
}


void CoreEntryEngineInterface::RunUserTriggedStop()
{
    // if the application has an OnStop function, run it
    if( m_pRunAplEntry->HasSpecialFunction(SpecialFunction::OnStop) )
    {
        m_pRunAplEntry->ExecSpecialFunction(SpecialFunction::OnStop, 0);

        bool bCancelStop = m_pRunAplEntry->HasSomeRequest() && !m_pRunAplEntry->IsEndingModifyMode();

        ProcessPossibleRequests();

        if( bCancelStop || m_currentEntryPage == nullptr )
            return;

        // proceed as if logic executed stop(0)
        safe_delete(m_currentEntryPage);
        m_pRunAplEntry->GetEntryDriver()->Exit_Code = 0;
    }

    // if not OnStop but the case was modified, make sure that the user wants to stop
    else if( m_bCaseModified )
    {
        // determine what options to show
        struct StopOptions
        {
            enum class Action { PartialSave, Finish, AdvanceToEnd, Discard, Cancel };
            std::vector<Action> aActions;
            std::vector<std::vector<CString>*> aStopButtonTexts;

            ~StopOptions()
            {
                safe_delete_vector_contents(aStopButtonTexts);
            }

            void Add(Action eAction, int iMessageNumber)
            {
                aActions.push_back(eAction);

                CString csButtonText = MGF::GetMessageText(iMessageNumber);
                std::vector<CString>* paStopButtonText = new std::vector<CString>();
                paStopButtonText->push_back(csButtonText);
                aStopButtonTexts.push_back(paStopButtonText);
            }
        };

        StopOptions stopOptions;

        if( AllowsPartialSave() && m_pRunAplEntry->QidReady(m_pRunAplEntry->GetCurrentLevel()) )
            stopOptions.Add(StopOptions::Action::PartialSave, MGF::PartialSaveTitle);

        if( m_pRunAplEntry->InModifyMode() )
        {
            if( m_pRunAplEntry->IsPathOn() )
                stopOptions.Add(StopOptions::Action::AdvanceToEnd, MGF::AdvanceToEnd);

            else
                stopOptions.Add(StopOptions::Action::Finish, MGF::SaveChanges);
        }

        stopOptions.Add(StopOptions::Action::Discard, MGF::DiscardChanges);
        stopOptions.Add(StopOptions::Action::Cancel, MGF::Cancel);

        CString csDialogTitle = MGF::GetMessageText(m_pRunAplEntry->InAddMode() ?
            MGF::StopAddingTitle : MGF::StopModifyingTitle);

        int iStopChoice = PlatformInterface::GetInstance()->GetApplicationInterface()->ShowChoiceDialog(csDialogTitle, stopOptions.aStopButtonTexts);

        StopOptions::Action eAction = ( iStopChoice == 0 ) ? StopOptions::Action::Cancel :
            (StopOptions::Action)stopOptions.aActions[iStopChoice - 1];

        if( eAction == StopOptions::Action::Cancel )
            return;

        else if( eAction == StopOptions::Action::AdvanceToEnd )
        {
            AdvanceToEnd();
            return;
        }

        else if( eAction == StopOptions::Action::Finish )
        {
            EndLevel();
            return;
        }

        else if( eAction == StopOptions::Action::PartialSave )
            PartialSave(false, true);
    }

    // stop (when the case wasn't modified or they selected Discard or Partial Save)
    OnStop();
}


void CoreEntryEngineInterface::ChangeLanguage()
{
    if( UseHtmlDialogs() )
    {
        if( m_pRunAplEntry->ChangeLanguage() )
            ProcessPossibleRequests();

        return;
    }

    // get the list of languages
    std::vector<Language> languages = m_pRunAplEntry->GetLanguages();

    if (languages.size() < 2)
    {
        // there are not multiple languages
        ShowModalDialog(_T(""), MGF::GetMessageText(MGF::SelectLanguageOnlyOneDefined), MB_OK);
        return;
    }

    std::vector<std::vector<CString>*> aLanguageLabels;

    for( const Language& language : languages )
    {
        std::vector<CString>* paThisLanguageLabel = new std::vector<CString>;
        paThisLanguageLabel->emplace_back(WS2CS(language.GetLabel()));
        aLanguageLabels.emplace_back(paThisLanguageLabel);
    }

    // display the language labels
    CString csDialogTitle = MGF::GetMessageText(MGF::SelectLanguageTitle);

    int iLanguageChoice = PlatformInterface::GetInstance()->GetApplicationInterface()->ShowChoiceDialog(csDialogTitle, aLanguageLabels);

    safe_delete_vector_contents(aLanguageLabels);

    // don't change the language if they canceled the dialog
    if( iLanguageChoice == 0 )
        return;

    // change the language
    m_pRunAplEntry->SetCurrentLanguage(languages[iLanguageChoice - 1].GetName());

    if( m_pRunAplEntry->HasSpecialFunction(SpecialFunction::OnChangeLanguage) )
        m_pRunAplEntry->ExecSpecialFunction(SpecialFunction::OnChangeLanguage, 0);

    ProcessPossibleRequests();
}


void CoreEntryEngineInterface::ExecuteCallbackUserFunction(UserFunctionArgumentEvaluator& user_function_argument_evaluator)
{
    m_pRunAplEntry->ExecuteCallbackUserFunction(m_currentEntryPage->GetField()->GetSymbol(), user_function_argument_evaluator);
    ProcessPossibleRequests();
}


bool CoreEntryEngineInterface::HasPersistentFields() const
{
    return m_pRunAplEntry->GetEntryDriver()->HasPersistentFields();
}


void CoreEntryEngineInterface::ProcessPossibleRequests()
{
    CDEField* next_field = m_currentEntryPage->GetField();

    // returns true if the program should be stopped
    if( m_pRunAplEntry->HasSomeRequest() )
    {
        m_pRunAplEntry->SetProgressForPreEntrySkip();
        next_field = (CDEField*)m_pRunAplEntry->RunCsDriver(true);
    }

    if( m_pRunAplEntry->IsStopRequestedAndTurnOffStop() )
    {
        safe_delete(m_currentEntryPage);
        OnStop();
    }

    else
        ProcessFieldPostMovement(next_field);
}


bool CoreEntryEngineInterface::GetShowCaseTreeFlag() const
{
    return m_pPifFile->GetApplication()->GetShowCaseTree();
}


bool CoreEntryEngineInterface::GetAutoAdvanceOnSelectionFlag() const
{
    return m_pPifFile->GetApplication()->GetAutoAdvanceOnSelection();
}


bool CoreEntryEngineInterface::GetDisplayCodesAlongsideLabelsFlag() const
{
    return m_pPifFile->GetApplication()->GetDisplayCodesAlongsideLabels();
}


const std::vector<CoreEntryFieldNote>& CoreEntryEngineInterface::GetAllNotes()
{
    ASSERT(false); // no longer used

    const DICT* pDicT = DIP(0);
    const DICX* pDicX = pDicT->GetDicX();
    const auto& notes = pDicX->GetCase().GetNotes();

    m_fieldNotes.clear();

    // convert from Note to CoreEntryFieldNote
    for( size_t index = 0; index < notes.size(); ++index )
    {
        const auto& note = notes[index];
        const auto& named_reference = note.GetNamedReference();

        // only show notes on the first level
        if( !named_reference.GetLevelKey().IsEmpty() )
            continue;

        CoreEntryFieldNote& field_note = m_fieldNotes.emplace_back();
        field_note.case_notes_index = index;
        field_note.note = note.GetContent();
        field_note.operator_id = note.GetOperatorId();

        const auto case_item_reference = dynamic_cast<const CaseItemReference*>(&named_reference);

        if( case_item_reference == nullptr )
        {
            field_note.is_field_note = false;
            field_note.group_symbol_index = -1;

            if( SO::Equals(named_reference.GetName(), pDicT->GetName()) )
            {
                field_note.label = _T("Case Note");
                field_note.sort_index = _T("!");
            }

            else
            {
                field_note.label = named_reference.GetName();
                field_note.sort_index.Format(_T("#%s"), named_reference.GetName().GetString());
            }
        }

        else
        {
            const VART* field_vart = VPT(case_item_reference->GetCaseItem().GetDictionaryItem().GetSymbol());
            const GROUPT* record_group = field_vart->GetOwnerGPT();

            field_note.is_field_note = true;
            field_note.label = field_vart->GetDictItem()->GetLabel();

            field_note.group_symbol_index = record_group->GetSymbolIndex();
            field_note.group_label = record_group->GetLabel();

            // get the record and item occurrence labels
            const TCHAR* occurrence_label_formatter = _T(" (%s)");
            std::vector<std::wstring> occurrence_labels = m_pEngineDriver->GetOccurrenceLabels(field_vart, case_item_reference);

            if( !occurrence_labels[0].empty() )
                field_note.group_label.AppendFormat(occurrence_label_formatter, occurrence_labels[0].c_str());

            if( !occurrence_labels[1].empty() )
            {
                field_note.label.AppendFormat(occurrence_label_formatter, occurrence_labels[1].c_str());
            }

            // add the item occurrence numbers if there is no occurrence label
            else
            {
                field_note.label.Append(case_item_reference->GetMinimalOccurrencesText());
            }


            // have this note sorted in the order that it appears in the data entry application; this
            // calculation might not work in all situations but it is good enough for these purposes
            const GROUPT* item_group = nullptr;

            if( record_group->GetDimType() == CDimension::VDimType::Item || record_group->GetDimType() == CDimension::VDimType::SubItem )
            {
                item_group = record_group;
                record_group = record_group->GetOwnerGPT();
            }

            const TCHAR* FlowOrderFormatter = _T("%07d");
            const TCHAR* OccurrenceFormatter = _T("%05d");

            field_note.sort_index.Format(FlowOrderFormatter, record_group->GetAbsoluteFlowOrder());
            field_note.sort_index.AppendFormat(OccurrenceFormatter, (int)case_item_reference->GetRecordOccurrence());

            if( item_group != nullptr )
            {
                size_t item_subitem_occurrence = case_item_reference->GetItemIndexHelper().HasSubitemOccurrences() ?
                    case_item_reference->GetSubitemOccurrence() : case_item_reference->GetItemOccurrence();

                field_note.sort_index.AppendFormat(FlowOrderFormatter, item_group->GetAbsoluteFlowOrder());
                field_note.sort_index.AppendFormat(OccurrenceFormatter, (int)item_subitem_occurrence);
            }

            field_note.sort_index.AppendFormat(FlowOrderFormatter, field_vart->GetAbsoluteFlowOrder());
        }
    }

    std::sort(m_fieldNotes.begin(), m_fieldNotes.end(), [](const CoreEntryFieldNote& note1, const CoreEntryFieldNote& note2)
    {
        return ( note1.sort_index.Compare(note2.sort_index) < 0 );
    });

    for( size_t index = 0; index < m_fieldNotes.size(); ++index )
        m_fieldNotes[index].index = index;

    return m_fieldNotes;
}


void CoreEntryEngineInterface::EditCaseNote()
{
    if( m_pEngineDriver->EditNote(true) )
        SetCaseModified();
}


void CoreEntryEngineInterface::ReviewNotes()
{
    std::shared_ptr<const CaseItemReference> field_to_goto = m_pRunAplEntry->ReviewNotes();

    if( field_to_goto != nullptr )
    {
        CDEField* pField = (CDEField*)m_pRunAplEntry->MoveToField(*field_to_goto, true);
        ProcessFieldPostMovement(pField);
    }
}


void CoreEntryEngineInterface::DeleteNote(size_t index)
{
    size_t case_notes_index = m_fieldNotes[index].case_notes_index;

    DICX* pDicX = DIX(0);
    const auto& note = pDicX->GetCase().GetNotes()[case_notes_index];

    m_pEngineDriver->DeleteNote_pre80(pDicX, note);
    SetCaseModified();

    // modify all of the indices to adjust for the deleted note
    for( auto& field_note : m_fieldNotes )
    {
        if( field_note.case_notes_index > case_notes_index )
            --field_note.case_notes_index;
    }
}


CoreEntryPage* CoreEntryEngineInterface::GoToNoteField(size_t index)
{
    size_t case_notes_index = m_fieldNotes[index].case_notes_index;
    ASSERT(m_fieldNotes[index].is_field_note);

    DICX* pDicX = DIX(0);
    const auto& note = pDicX->GetCase().GetNotes()[case_notes_index];

    const auto& case_item_reference = assert_cast<const CaseItemReference&>(note.GetNamedReference());

    auto the3dObject = m_pIntDriver->ConvertIndex(case_item_reference);
    DEFLD cField(the3dObject->GetSymbol(), the3dObject->GetIndexes());

    CDEField* pField = (CDEField*)m_pRunAplEntry->MoveToField(&cField, true);

    return ProcessFieldPostMovement(pField);
}


int CoreEntryEngineInterface::ShowModalDialog(CString sTitle, CString sMessage, int mbType)
{
    return PlatformInterface::GetInstance()->GetApplicationInterface()->ShowModalDialog(sTitle, sMessage, mbType);
}


bool CoreEntryEngineInterface::AllowsPartialSave() const
{
    return m_pPifFile->GetApplication()->GetPartialSave();
}


bool CoreEntryEngineInterface::PartialSave(bool bClearSkipped/* = false*/, bool bFromLogic/* = false*/)
{
    bool bSaved = false;

    if( m_pRunAplEntry->QidReady(m_pRunAplEntry->GetCurrentLevel()) )
    {
        APP_MODE eAppMode =
            ( m_pRunAplEntry->GetAppMode() == CRUNAPL_ADD ) ? ADD_MODE :
            ( m_pRunAplEntry->GetAppMode() == CRUNAPL_MODIFY ) ? MODIFY_MODE :
            VERIFY_MODE;

        bSaved = m_pRunAplEntry->PartialSaveCase(eAppMode, bClearSkipped);

        if( bSaved )
            SetCaseModified(false);
    }

    if( !bFromLogic || !bSaved )
    {
        // display a message regarding the success
        int iMessageNumber = bSaved ? MGF::PartialSaveSuccess : MGF::PartialSaveFailure;
        ShowModalDialog(MGF::GetMessageText(MGF::PartialSaveTitle), MGF::GetMessageText(iMessageNumber), MB_OK);
    }

    return bSaved;
}


bool CoreEntryEngineInterface::GetShowRefusalsFlag() const
{
    return m_pPifFile->GetApplication()->GetShowRefusals();
}


bool CoreEntryEngineInterface::ShowRefusedValues()
{
    ASSERT(!GetShowRefusalsFlag());
    return m_pRunAplEntry->ShowRefusalProcessor(CRunAplEntry::ShowRefusalProcessorAction::ShowRefusedValues, true);
}


bool CoreEntryEngineInterface::HasSync() const
{
    const AppSyncParameters& syncParams = m_pPifFile->GetApplication()->GetSyncParameters();
    return !syncParams.server.empty();
}

bool CoreEntryEngineInterface::SyncApp()
{
    const AppSyncParameters& syncParams = m_pPifFile->GetApplication()->GetSyncParameters();
    return m_pRunAplEntry->RunSync(syncParams);
}


DeploymentPackageDownloader* CoreEntryEngineInterface::CreateDeploymentPackageDownloader()
{
    return new DeploymentPackageDownloader();
}


void CoreEntryEngineInterface::ProcessParadataCachedEvents(const std::vector<CString>& event_strings)
{
    if (m_pIntDriver && m_pIntDriver->m_pParadataDriver)
        m_pIntDriver->m_pParadataDriver->ProcessCachedEvents(event_strings);
}


CoreEntryEngineInterface::PffStartModeParameter CoreEntryEngineInterface::QueryPffStartMode()
{
    PffStartModeParameter parameter;
    parameter.action = PffStartModeParameter::Action::NoAction;

    enum class KeyToOpenMode { None, Key, StartModeAdd, StartModeModify };
    KeyToOpenMode key_to_open_mode = KeyToOpenMode::None;
    CString key_to_open;

    // StartMode will take precedence over Key
    if( m_pPifFile->GetStartMode() != StartMode::None )
    {
        if( m_pPifFile->GetStartMode() == StartMode::Add )
            key_to_open_mode = KeyToOpenMode::StartModeAdd;

        else if( m_pPifFile->GetStartMode() == StartMode::Modify )
            key_to_open_mode = KeyToOpenMode::StartModeModify;

        if( key_to_open_mode != KeyToOpenMode::None )
        {
            key_to_open = m_pPifFile->GetStartKeyString();
            key_to_open.Trim();
        }
    }

    else
    {
        key_to_open = GetStartPffKey();

        if( !key_to_open.IsEmpty() )
            key_to_open_mode = KeyToOpenMode::Key;
    }

    // find the proper key to open
    if( key_to_open_mode != KeyToOpenMode::None )
    {
        // add is fine regardless of whether or not a key is provided and found but
        // not supplying a key is not okay in modify mode
        if( key_to_open_mode == KeyToOpenMode::StartModeModify && key_to_open.IsEmpty() )
            parameter.action = PffStartModeParameter::Action::ModifyError;

        // lookup the key to get the file position
        else
        {
            DataRepository* pInputRepo = m_pRunAplEntry->GetInputRepository();

            try
            {
                CString uuid;
                double position_in_repository;

                pInputRepo->PopulateCaseIdentifiers(key_to_open, uuid, position_in_repository);

                parameter.action = PffStartModeParameter::Action::ModifyCase;
                parameter.modify_case_position = position_in_repository;
            }

            catch( const DataRepositoryException::CaseNotFound& ) { }

            if( parameter.action != PffStartModeParameter::Action::ModifyCase )
            {
                if( key_to_open_mode == KeyToOpenMode::StartModeModify )
                    parameter.action = PffStartModeParameter::Action::ModifyError;

                else if( ( key_to_open_mode == KeyToOpenMode::StartModeAdd ) ||
                         ( key_to_open_mode == KeyToOpenMode::Key && key_to_open.GetLength() >= m_pRunAplEntry->GetInputDictionaryKeyLength() ) )
                {
                    parameter.action = PffStartModeParameter::Action::AddNewCase;
                }
            }
        }
    }

    return parameter;
}


CString CoreEntryEngineInterface::GetStartPffKey() const
{
    CString key = m_pPifFile->GetKey();
    return ( key.GetLength() >= m_pRunAplEntry->GetInputDictionaryKeyLength() ) ? key : CString();
}


bool CoreEntryEngineInterface::DoNotShowCaseListing() const
{
    return ( m_pPifFile->GetCaseListingLockFlag() || m_pPifFile->GetKey().GetLength() >= m_pRunAplEntry->GetInputDictionaryKeyLength() );
}


std::shared_ptr<CaseTreeNode> CoreEntryEngineInterface::GetCaseTree()
{
    m_caseTree = m_caseTreeBuilder->build(m_pRunAplEntry->GetFormFileInProcess(), m_currentEntryPage);
    return m_caseTree;
}


std::vector<CaseTreeUpdate> CoreEntryEngineInterface::UpdateCaseTree()
{
    return m_caseTreeBuilder->update(m_caseTree, m_currentEntryPage);
}


// methods for dealing with system settings
CommonStore* CoreEntryEngineInterface::GetCommonStore()
{
    return ( m_pEngineDriver != nullptr ) ? m_pEngineDriver->GetCommonStore().get() : nullptr;
}


CString CoreEntryEngineInterface::GetSystemSetting(wstring_view setting_name, wstring_view default_value)
{
    CString setting_value = CommonStore::GetSystemSetting(setting_name);
    return setting_value.IsEmpty() ? CString(default_value) : setting_value;
}


bool CoreEntryEngineInterface::GetSystemSetting(wstring_view setting_name, bool default_value)
{
    CString setting_value = CommonStore::GetSystemSetting(setting_name);
    return setting_value.IsEmpty() ? default_value : ( setting_value.CompareNoCase(_T("Yes")) == 0 );
}


const AppMappingOptions& CoreEntryEngineInterface::GetMappingOptions() const
{
    return m_pPifFile->GetApplication()->GetMappingOptions();
}
