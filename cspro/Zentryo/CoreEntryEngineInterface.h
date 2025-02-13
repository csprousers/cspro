#pragma once

#include <Zentryo/zEntryO.h>
#include <Zentryo/CoreEntryFieldNote.h>
#include <Zentryo/Runaple.h>
#include <Zentryo/CaseTreeNode.h>
#include <Zentryo/CaseTreeUpdate.h>
#include <Zentryo/DeploymentPackageDownloader.h>
#include <zFormO/FormFile.h>

class ApplicationInterface;
struct AppMappingOptions;
class CaseTreeBuilder;
class CDataDict;
class CDEGroup;
class CDictItem;
class CEngineArea;
class CEntryDriver;
class CIntDriver;
class CNPifFile;
class CommonStore;
class CoreEntryPage;
struct EngineData;
class ObjectTransporter;
class UserFunctionArgumentEvaluator;


///<summary>
///Core interface from UI layer into engine for data entry application.
///Currently this is only used for Android & Win Universal platforms.
///Windows desktop has a totally different way to access the engine.
///</summary>
class CLASS_DECL_ZENTRYO CoreEntryEngineInterface
{
public:
    CoreEntryEngineInterface();
    ~CoreEntryEngineInterface();

    ObjectTransporter* GetObjectTransporter();

    ///<summary>
    ///Open a CSEntry application
    ///</summary>
    bool InitApplication(const CString& pff_filename);

    CNPifFile* GetPifFile() const { return m_pPifFile; }

    bool Start(std::optional<double> insert_before_position_in_repository = std::nullopt);
    bool ModifyCase(double position_in_repository);
    bool InsertCase(double insert_before_position_in_repository);
    bool DeleteCase(double position_in_repository);

    /// <summary>
    /// Gets the case keys for the input repository.
    /// </summary>

    struct CaseSummaryWithLatLong
    {
        CaseSummary case_summary;
        double latitude;
        double longitude;
    };

    std::vector<CaseSummaryWithLatLong> GetSequentialCaseIds(bool sort_ascending = true,
        const std::optional<CString>& lat_item_name = {},
        const std::optional<CString>& lon_item_name = {});

    CoreEntryPage* GetCurrentEntryPage() { return m_currentEntryPage; }

    CoreEntryPage* EndGroup();
    CoreEntryPage* EndLevel();
    CoreEntryPage* EndLevelOcc();
    CoreEntryPage* AdvanceToEnd();

    CoreEntryPage* NextField();
    CoreEntryPage* PreviousField();
    CoreEntryPage* PreviousPersistentField();
    CoreEntryPage* GoToField(int fieldSymbol, const int index[3]);

    bool InsertOcc();
    bool InsertOccAfter();
    bool DeleteOcc();

    bool IsSystemControlled() const;
    bool ContainsMultipleLanguages() const;

    bool GetAskOpIDFlag() const;
    void SetOperatorId(CString operatorID);

    int GetStopCode() const;
    void OnStop();
    void RunUserTriggedStop();

    void ChangeLanguage();

    void ProcessPossibleRequests();

    void ExecuteCallbackUserFunction(UserFunctionArgumentEvaluator& user_function_argument_evaluator);

    bool HasPersistentFields() const;

    bool GetShowCaseTreeFlag() const;
    std::shared_ptr<CaseTreeNode> GetCaseTree();
    std::vector<CaseTreeUpdate> UpdateCaseTree();

    bool GetAutoAdvanceOnSelectionFlag() const;
    bool GetDisplayCodesAlongsideLabelsFlag() const;

    void SetCaseModified(bool modified = true) { m_bCaseModified = modified; }
    bool* GetCaseModifiedFlagIfNotModified() { return m_bCaseModified ? nullptr : &m_bCaseModified; }

    const std::vector<CoreEntryFieldNote>& GetAllNotes();
    void EditCaseNote();
    void ReviewNotes();
    void DeleteNote(size_t index);
    CoreEntryPage* GoToNoteField(size_t index);

    bool AllowsPartialSave() const;
    bool PartialSave(bool bClearSkipped = false, bool bFromLogic = false);

    bool GetShowRefusalsFlag() const;
    bool ShowRefusedValues();

    bool HasSync() const;
    bool SyncApp();

    DeploymentPackageDownloader* CreateDeploymentPackageDownloader();

    void ProcessParadataCachedEvents(const std::vector<CString>& event_strings);

    struct PffStartModeParameter
    {
        enum class Action { NoAction, AddNewCase, ModifyCase, ModifyError };
        Action action;
        double modify_case_position;
    };

    PffStartModeParameter QueryPffStartMode();
    CString GetStartPffKey() const;
    bool DoNotShowCaseListing() const;

    CommonStore* GetCommonStore();
    static CString GetSystemSetting(wstring_view setting_name, wstring_view default_value);
    static bool GetSystemSetting(wstring_view setting_name, bool default_value);

    CRunAplEntry* GetRunAplEntry() { return m_pRunAplEntry; }

    const AppMappingOptions& GetMappingOptions() const;

    void Cleanup();

private:
    const Logic::SymbolTable& GetSymbolTable() const;

    CoreEntryPage* ProcessFieldPostMovement(CDEField* pField);
    bool ProcessOccurrenceModification(CDEItemBase* (CRunAplEntry::*pOccurrenceModificationFunction)(bool& bRet));

    int ShowModalDialog(CString sTitle, CString sMessage, int mbType);

private:
    std::optional<std::tuple<const CEntryDriver*, std::unique_ptr<ObjectTransporter>>> m_objectTransporter;

    CNPifFile* m_pPifFile;
    CRunAplEntry* m_pRunAplEntry;

    CoreEntryPage* m_currentEntryPage;
    bool m_bCaseModified;

    std::shared_ptr<CaseTreeNode> m_caseTree;
    CaseTreeBuilder* m_caseTreeBuilder;

    std::vector<CoreEntryFieldNote> m_fieldNotes;

    // the following are here for convenience reasons and link to objects within m_pRunAplEntry
    CEntryDriver* m_pEngineDriver;
    EngineData* m_engineData;
    CIntDriver* m_pIntDriver;
};
