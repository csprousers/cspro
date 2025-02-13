#pragma once

#include <zAppO/zAppO.h>
#include <zAppO/AppSyncParameters.h>
#include <zAppO/CodeFile.h>
#include <zAppO/DictionaryDescription.h>
#include <zAppO/LogicSettings.h>
#include <zAppO/MappingDefines.h>
#include <zUtilO/TextSource.h>

class ApplicationLoader;
class ApplicationProperties;
class CapiQuestionManager;
class CAppLoader;
class CDataDict;
class CDEFormFile;
class CSourceCode;
class CSpecFile;
class CTabSet;
class Serializer;
namespace JsonSpecFile { class ReaderMessageLogger; }


enum class EngineAppType : int { Invalid = -1, Entry = 1, Tabulation, Batch };
ZAPPO_API const TCHAR* const ToString(EngineAppType application_type);

enum class CaseTreeType : int { Never = -1, MobileOnly, DesktopOnly, Always };

namespace EditNotePermissions
{
    constexpr int DeleteOtherOperators = 0x01;
    constexpr int EditOtherOperators   = 0x02;
    constexpr int AllPermissions       = 0xFFFFFFFF;
};


class ZAPPO_API Application
{
public:
    Application();
    Application(Application&& rhs) noexcept;
    Application(const Application& rhs) = delete;
    ~Application();

    // application properties
    // --------------------------------------------------------------------------
    double GetVersion() const               { return m_version; }
    int GetSerializerArchiveVersion() const { return m_serializerArchiveVersion; }

    EngineAppType GetEngineAppType() const               { return m_engineAppType; }
    void SetEngineAppType(EngineAppType engine_app_type) { m_engineAppType = engine_app_type; }

    const CString& GetName() const    { return m_name; }
    void SetName(const CString& name) { m_name = name; }
    void SetName(std::wstring name)   { m_name = WS2CS(name); }

    void SetLabel(const CString& label) { m_label = label; }
    void SetLabel(std::wstring label)   { m_label = WS2CS(label); }
    const CString& GetLabel() const     { return m_label; }

    const CString& GetApplicationFilename() const        { return m_applicationFilename; }
    void SetApplicationFilename(const CString& filenane) { m_applicationFilename = filenane; }


    // when using an application properties file, the properties will be read on Open but will not be saved on Save,
    // so modifying properties using the non-const version of GetApplicationProperties or SetApplicationProperties
    // must be done with care
    const std::wstring& GetApplicationPropertiesFilename() const { return m_applicationPropertiesFilename; }
    void SetApplicationPropertiesFilename(std::wstring filename) { m_applicationPropertiesFilename = std::move(filename); }

    const ApplicationProperties& GetApplicationProperties() const { return *m_applicationProperties; }
    ApplicationProperties& GetApplicationProperties()             { return *m_applicationProperties; }
    void SetApplicationProperties(ApplicationProperties application_properties);


    // other filenames
    // --------------------------------------------------------------------------
    const CString& GetQuestionTextFilename() const        { return m_questionTextFilename; }
    void SetQuestionTextFilename(const CString& filenane) { m_questionTextFilename = filenane; }


    // form files
    // --------------------------------------------------------------------------
    const std::vector<CString>& GetFormFilenames() const { return m_formFilenames; }

    void AddFormFilename(const CString& form_filename);
    void AddFormFilename(std::wstring form_filename);
    void DropFormFilename(wstring_view form_filename);
    void RenameFormFilename(wstring_view original_form_filename, const CString& new_form_filename);


    // tab specs
    // --------------------------------------------------------------------------
    const std::vector<CString>& GetTabSpecFilenames() const { return m_tabSpecFilenames; }

    void AddTabSpecFilename(const CString& tab_spec_filename);
    void RenameTabSpecFilename(wstring_view original_tab_spec_filename, const CString& new_tab_spec_filename);


    // external dictionaries
    // --------------------------------------------------------------------------
    const std::vector<CString>& GetExternalDictionaryFilenames() const { return m_externalDictionaryFilenames; }

    void AddExternalDictionaryFilename(const CString& dictionary_filename);
    void DropExternalDictionaryFilename(wstring_view dictionary_filename);
    void RenameExternalDictionaryFilename(wstring_view original_dictionary_filename, std::wstring new_dictionary_filename);


    // code files
    // --------------------------------------------------------------------------
    const std::vector<CodeFile>& GetCodeFiles() const { return m_codeFiles; }
    auto GetCodeFilesIterator()                       { return VI_V(m_codeFiles); }

    const CodeFile* GetLogicMainCodeFile() const;
    CodeFile* GetLogicMainCodeFile();

    void AddCodeFile(CodeFile code_file);
    void DropCodeFile(size_t index);


    // message files
    // --------------------------------------------------------------------------
    const std::vector<std::shared_ptr<TextSource>>& GetMessageTextSources() const { return m_messageTextSources; }

    void AddMessageFile(std::shared_ptr<TextSource> message_text_source);
    void DropMessageFile(size_t index);


    // reports
    // --------------------------------------------------------------------------
    const std::vector<std::shared_ptr<NamedTextSource>>& GetReportNamedTextSources() const { return m_reportNamedTextSources; }

    const NamedTextSource* GetReportNamedTextSource(wstring_view name_or_filename, bool search_by_name) const;
    void AddReport(std::wstring name, std::shared_ptr<TextSource> report_text_source);
    void AddReport(std::wstring name, std::wstring filename);
    void DropReport(size_t index);


    // resource folders
    // --------------------------------------------------------------------------
    const std::vector<CString>& GetResourceFolders() const { return m_resourceFolders; }

    void AddResourceFolder(const CString& folder);
    void DropResourceFolder(size_t index);


    // dictionary descriptions
    // --------------------------------------------------------------------------
    const std::vector<DictionaryDescription>& GetDictionaryDescriptions() const                   { return m_dictionaryDescriptions; }
    std::vector<DictionaryDescription>& GetDictionaryDescriptions()                               { return m_dictionaryDescriptions; }

    void SetDictionaryDescriptions(std::vector<DictionaryDescription> dictionary_descriptions)    { m_dictionaryDescriptions = std::move(dictionary_descriptions); }
    DictionaryDescription* AddDictionaryDescription(DictionaryDescription dictionary_description) { return &m_dictionaryDescriptions.emplace_back(std::move(dictionary_description)); }

    DictionaryType GetDictionaryType(const CDataDict& dictionary) const;
    const DictionaryDescription* GetDictionaryDescription(wstring_view dictionary_filename, wstring_view parent_filename = wstring_view()) const;
    DictionaryDescription* GetDictionaryDescription(wstring_view dictionary_filename, wstring_view parent_filename = wstring_view());
    const std::wstring& GetFirstDictionaryFilenameOfType(DictionaryType dictionary_type) const;


    // flags
    // --------------------------------------------------------------------------
    bool GetAskOperatorId() const    { return m_askOperatorId; }
    void SetAskOperatorId(bool flag) { m_askOperatorId = flag; }

    bool GetPartialSave() const    { return m_partialSave; }
    void SetPartialSave(bool flag) { m_partialSave = flag; }

    int GetAutoPartialSaveMinutes() const       { return m_autoPartialSaveMinutes; }
    bool GetAutoPartialSave() const             { return ( m_autoPartialSaveMinutes > 0 ); }
    void SetAutoPartialSaveMinutes(int minutes) { m_autoPartialSaveMinutes = SetMinutesVariable(minutes); }

    CaseTreeType GetCaseTreeType() const              { return m_caseTreeType; }
    bool GetShowCaseTree() const;
    void SetCaseTreeType(CaseTreeType case_tree_type) { m_caseTreeType = case_tree_type; }

    bool GetUseQuestionText() const    { return m_useQuestionText; }
    void SetUseQuestionText(bool flag) { m_useQuestionText = flag; }

    bool GetShowEndCaseMessage() const    { return m_showEndCaseMessage; }
    void SetShowEndCaseMessage(bool flag) { m_showEndCaseMessage = flag; }

    bool GetCenterForms() const    { return m_centerForms; }
    void SetCenterForms(bool flag) { m_centerForms = flag; }

    bool GetDecimalMarkIsComma() const    { return m_decimalMarkIsComma; }
    void SetDecimalMarkIsComma(bool flag) { m_decimalMarkIsComma = flag; }

    bool GetCreateListingFile() const    { return m_createListingFile; }
    void SetCreateListingFile(bool flag) { m_createListingFile = flag; }

    bool GetCreateLogFile() const    { return m_createLogFile; }
    void SetCreateLogFile(bool flag) { m_createLogFile = flag; }

    bool GetEditNotePermissions(int permission) const { return ( ( m_editNotePermissions & permission ) != 0 ); }
    void SetEditNotePermissions(int permission, bool flag);

    bool GetAutoAdvanceOnSelection() const    { return m_autoAdvanceOnSelection; }
    void SetAutoAdvanceOnSelection(bool flag) { m_autoAdvanceOnSelection = flag; }

    bool GetDisplayCodesAlongsideLabels() const    { return m_displayCodesAlongsideLabels; }
    void SetDisplayCodesAlongsideLabels(bool flag) { m_displayCodesAlongsideLabels = flag; }

    bool GetShowFieldLabels() const    { return m_showFieldLabels; }
    void SetShowFieldLabels(bool flag) { m_showFieldLabels = flag; }

    bool GetShowErrorMessageNumbers() const    { return m_showErrorMessageNumbers; }
    void SetShowErrorMessageNumbers(bool flag) { m_showErrorMessageNumbers = flag; }

    bool GetComboBoxShowOnlyDiscreteValues() const    { return m_comboBoxShowOnlyDiscreteValues; }
    void SetComboBoxShowOnlyDiscreteValues(bool flag) { m_comboBoxShowOnlyDiscreteValues = flag; }

    bool GetShowRefusals() const    { return m_showRefusals; }
    void SetShowRefusals(bool flag) { m_showRefusals = flag; }

    static constexpr int GetVerifyFreqMax() { return 99; }
    int GetVerifyFreq() const               { return m_verifyFrequency; }
    void SetVerifyFreq(int frequency)       { m_verifyFrequency = frequency; }

    int GetVerifyStart() const     { return m_verifyStart; }
    void SetVerifyStart(int start) { m_verifyStart = start; }

    const AppSyncParameters& GetSyncParameters() const { return m_syncParameters; }
    void SetSyncParameters(AppSyncParameters params)   { m_syncParameters = std::move(params); }

    const LogicSettings& GetLogicSettings() const       { return m_logicSettings; }
    void SetLogicSettings(LogicSettings logic_settings) { m_logicSettings = std::move(logic_settings); }

    const AppMappingOptions& GetMappingOptions() const { return m_mappingOptions; }
    void SetMappingOptions(AppMappingOptions options)  { m_mappingOptions = std::move(options); }


    // flags set during compilation
    // --------------------------------------------------------------------------
    bool GetHasWriteStatements() const             { return m_hasWriteStatements; }
    void SetHasWriteStatements()                   { m_hasWriteStatements = true; }

    bool GetHasSaveableFrequencyStatements() const { return m_hasSaveableFrequencyStatements; }
    void SetHasSaveableFrequencyStatements()       { m_hasSaveableFrequencyStatements = true; }

    bool GetHasImputeStatements() const            { return m_hasImputeStatements; }
    void SetHasImputeStatements()                  { m_hasImputeStatements = true; }

    bool GetHasImputeStatStatements() const        { return m_hasImputeStatStatements; }
    void SetHasImputeStatStatements()              { m_hasImputeStatStatements = true; }

    bool GetHasSaveArrays() const                  { return m_hasSaveArrays; }
    void SetHasSaveArrays()                        { m_hasSaveArrays = true; }

    bool GetUpdateSaveArrayFile() const            { return m_updateSaveArrayFile; }
    void SetUpdateSaveArrayFile(bool update)       { m_updateSaveArrayFile = update; }


    // serialization
    // --------------------------------------------------------------------------

    // all serialization methods (with the exception of WriteJson and serialize) can throw exceptions
    void Open(NullTerminatedString filename, bool silent = false, bool load_text_sources_and_external_application_properties = true);
    void Save(NullTerminatedString filename, bool continue_using_filename = true) const;

    static Application CreateFromJson(const JsonNode<wchar_t>& json_node);
    void WriteJson(JsonWriter& json_writer, bool write_to_new_json_object = true) const;

    void serialize(Serializer& ar);

private:
    void CreateFromJsonWorker(const JsonNode<wchar_t>& json_node, bool load_text_sources_and_external_application_properties,
                              bool silent, std::shared_ptr<JsonSpecFile::ReaderMessageLogger> message_logger);

    static std::wstring ConvertPre80SpecFile(NullTerminatedString filename);


public:
    // miscellaneous
    // --------------------------------------------------------------------------
    CSourceCode* GetAppSrcCode()                 { return m_pAppSrcCode; }
    void SetAppSrcCode(CSourceCode* pAppSrcCode) { m_pAppSrcCode = pAppSrcCode; }

    bool IsCompiled() const     { return m_compiled; }
    void SetCompiled(bool flag) { m_compiled = flag; }

    bool GetOptimizeFlowTree() const    { return m_optimizeFlowTree; }
    void SetOptimizeFlowTree(bool flag) { m_optimizeFlowTree = flag; }

    CAppLoader* GetAppLoader() { return m_pAppLoader.get(); }

    ApplicationLoader* GetApplicationLoader()                                        { return m_applicationLoader.get(); }
    void SetApplicationLoader(std::shared_ptr<ApplicationLoader> application_loader) { m_applicationLoader = std::move(application_loader); }


    // objects used at runtime
    // --------------------------------------------------------------------------
    const std::vector<std::shared_ptr<CDataDict>>& GetRuntimeExternalDictionaries() const { return m_runtimeExternalDictionaries; }
    std::vector<std::shared_ptr<CDataDict>>& GetRuntimeExternalDictionaries()             { return m_runtimeExternalDictionaries; }
    void AddRuntimeExternalDictionary(std::shared_ptr<CDataDict> dictionary)              { m_runtimeExternalDictionaries.emplace_back(std::move(dictionary)); }

    const std::vector<std::shared_ptr<CDEFormFile>>& GetRuntimeFormFiles() const { return m_runtimeFormFiles; }
    std::vector<std::shared_ptr<CDEFormFile>>& GetRuntimeFormFiles()             { return m_runtimeFormFiles; }
    void AddRuntimeFormFile(std::shared_ptr<CDEFormFile> form_file)              { m_runtimeFormFiles.emplace_back(std::move(form_file)); }

    std::shared_ptr<const CapiQuestionManager> GetCapiQuestionManager() const          { return m_questionManager; }
    std::shared_ptr<CapiQuestionManager> GetCapiQuestionManager()                      { return m_questionManager; }
    void SetCapiQuestionManager(std::shared_ptr<CapiQuestionManager> question_manager) { m_questionManager = std::move(question_manager); }

    std::shared_ptr<CTabSet> GetTabSpec()              { return m_pTableSpec; }
    void SetTabSpec(std::shared_ptr<CTabSet> pTabSpec) { m_pTableSpec = std::move(pTabSpec); }


    // other methods
    // --------------------------------------------------------------------------

    // returns false if the name is used by a report or code namespace
    bool IsNameUnique(const std::wstring& name) const;

    static constexpr int SetMinutesVariable(int minutes, int min_minutes = 0, int max_minutes = 360)
    {
        return std::max(min_minutes, std::min(minutes, max_minutes));
    }


private:
    // application properties
    double m_version;
    int m_serializerArchiveVersion;
    EngineAppType m_engineAppType;
    CString m_name;
    CString m_label;
    CString m_applicationFilename;

    std::wstring m_applicationPropertiesFilename;
    std::unique_ptr<ApplicationProperties> m_applicationProperties;

    // other filenames
    CString m_questionTextFilename;
    std::vector<CString> m_formFilenames;
    std::vector<CString> m_tabSpecFilenames;
    std::vector<CString> m_externalDictionaryFilenames;
    std::vector<CodeFile> m_codeFiles;
    std::vector<std::shared_ptr<TextSource>> m_messageTextSources;
    std::vector<std::shared_ptr<NamedTextSource>> m_reportNamedTextSources;
    std::vector<CString> m_resourceFolders;

    // other + flags
    std::vector<DictionaryDescription> m_dictionaryDescriptions;

    bool m_askOperatorId;
    bool m_partialSave;
    int m_autoPartialSaveMinutes;
    CaseTreeType m_caseTreeType;
    bool m_useQuestionText;
    bool m_showEndCaseMessage;
    bool m_centerForms;
    bool m_decimalMarkIsComma;
    bool m_createListingFile;
    bool m_createLogFile;
    int m_editNotePermissions;
    bool m_autoAdvanceOnSelection;
    bool m_displayCodesAlongsideLabels;
    bool m_showFieldLabels;
    bool m_showErrorMessageNumbers;
    bool m_comboBoxShowOnlyDiscreteValues;
    bool m_showRefusals;

    int m_verifyFrequency; // is within 1 and 99
    int m_verifyStart;     // -1 implies random //is within 1 and 99

    AppSyncParameters m_syncParameters;
    LogicSettings m_logicSettings;
    AppMappingOptions m_mappingOptions;

    // some flags set during compilation
    bool m_hasWriteStatements;
    bool m_hasSaveableFrequencyStatements;
    bool m_hasImputeStatements;
    bool m_hasImputeStatStatements;
    bool m_hasSaveArrays;
    bool m_updateSaveArrayFile;

    // miscellaneous
    CSourceCode* m_pAppSrcCode; // Source code object for the .app
    bool m_compiled;

    bool m_optimizeFlowTree;

    std::unique_ptr<CAppLoader> m_pAppLoader;   // mgr for file load
    std::shared_ptr<ApplicationLoader> m_applicationLoader;

    // objects used at runtime
    // APP_LOAD_TODO eventually the engine should get these from the ApplicationLoader
    std::vector<std::shared_ptr<CDataDict>> m_runtimeExternalDictionaries;
    std::vector<std::shared_ptr<CDEFormFile>> m_runtimeFormFiles;
    std::shared_ptr<CapiQuestionManager> m_questionManager;

    std::shared_ptr<CTabSet> m_pTableSpec; //Table spec used @ tab runtime
};
