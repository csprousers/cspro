#pragma once

#include <zAppO/zAppO.h>
#include <zAppO/AppMessageFile.h>
#include <zAppO/AppResource.h>
#include <zAppO/AppSyncParameters.h>
#include <zAppO/CodeFile.h>
#include <zAppO/DictionaryDescription.h>
#include <zAppO/LogicSettings.h>
#include <zAppO/MappingDefines.h>
#include <zAppO/ReportFile.h>

enum class AppFileType;
class ApplicationLoader;
class ApplicationProperties;
class CapiQuestionManager;
class CAppLoader;
class CDataDict;
class CDEFormFile;
class CSourceCode;
class CSpecFile;
class CTabSet;
namespace JsonSpecFile { class ReaderMessageLogger; }


enum class EngineAppType : int { Invalid = -1, Entry = 1, Tabulation, Batch };
ZAPPO_API const char* ToString(EngineAppType application_type);

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
    Application(const Application& rhs);
    ~Application();

    // application properties
    // --------------------------------------------------------------------------
    double GetVersion() const               { return m_version; }
    int GetSerializerArchiveVersion() const { return m_serializerArchiveVersion; }

    EngineAppType GetEngineAppType() const noexcept               { return m_engineAppType; }
    void SetEngineAppType(EngineAppType engine_app_type) noexcept { m_engineAppType = engine_app_type; }

    AppFileType GetApplicationAppFileType() const;

    const std::string& GetName() const { return m_name; }
    void SetName(std::string name)     { m_name = std::move(name); }

    const std::string& GetLabel() const { return m_label; }
    void SetLabel(std::string label)    { m_label = std::move(label); }

    const std::string& GetApplicationFilePath() const  { return m_applicationFilePath; }
    void SetApplicationFilePath(std::string file_path) { m_applicationFilePath = std::move(file_path); }


    // When using an application properties file, the properties will be read on Open but will not be saved on Save,
    // so modifying properties using the non-const version of GetApplicationProperties or SetApplicationProperties
    // must be done with care.
    const std::string& GetApplicationPropertiesFilePath() const  { return m_applicationPropertiesFilePath; }
    void SetApplicationPropertiesFilePath(std::string file_path) { m_applicationPropertiesFilePath = std::move(file_path); }

    const ApplicationProperties& GetApplicationProperties() const { return *m_applicationProperties; }
    ApplicationProperties& GetApplicationProperties()             { return *m_applicationProperties; }
    void SetApplicationProperties(ApplicationProperties application_properties);


    // miscellaneous functionality
    // --------------------------------------------------------------------------

    // Clears the values for the objects listed below under "other application files" as
    // well as the dictionary descriptions.
    void ClearApplicationFiles();


    // question text
    // --------------------------------------------------------------------------
    const std::string& GetQuestionTextFilePath() const  { return m_questionTextFilePath; }
    void SetQuestionTextFilePath(std::string file_path) { m_questionTextFilePath = std::move(file_path); }


    // form files
    // --------------------------------------------------------------------------
    const std::vector<std::string>& GetFormFilePaths() const { return m_formFilePaths; }

    const std::string* GetForm(const std::string& form_file_path);

    void AddForm(std::string form_file_path);
    void DropForm(const std::string& form_file_path);
    void RenameFormFilePath(const std::string& original_form_file_path, std::string new_form_file_path);


    // table specs
    // --------------------------------------------------------------------------
    const std::vector<std::string>& GetTableSpecFilePaths() const { return m_tableSpecFilePaths; }

    void AddTableSpec(std::string table_spec_file_path);
    void RenameTableSpecFilePath(const std::string& original_table_spec_file_path, std::string new_table_spec_file_path);


    // external dictionaries
    // --------------------------------------------------------------------------
    const std::vector<std::string>& GetExternalDictionaryFilePaths() const { return m_externalDictionaryFilePaths; }

    void AddExternalDictionary(std::string dictionary_file_path);
    void DropExternalDictionary(const std::string& dictionary_file_path);
    void RenameExternalDictionaryFilePath(const std::string& original_dictionary_file_path, std::string new_dictionary_file_path);


    // code files
    // --------------------------------------------------------------------------
    const std::vector<CodeFile>& GetCodeFiles() const { return m_codeFiles; }
    auto GetCodeFilesIterator()                       { return VI_V(m_codeFiles); }

    const CodeFile* GetCodeFile(const std::string& file_path) const;

    const CodeFile* GetLogicMainCodeFile() const;
    CodeFile* GetLogicMainCodeFile();

    void AddCodeFile(CodeFile code_file);
    void DropCodeFile(const std::string& file_path);


    // message files
    // --------------------------------------------------------------------------
    const std::vector<AppMessageFile>& GetMessageFiles() const { return m_messageFiles; }
    auto GetMessageFilesIterator()                             { return VI_V(m_messageFiles); }

    void AddMessageFile(AppMessageFile app_message_file);
    void DropMessageFile(const std::string& file_path);


    // reports
    // --------------------------------------------------------------------------
    const std::vector<ReportFile>& GetReportFiles() const { return m_reportFiles; }
    auto GetReportFilesIterator()                         { return VI_V(m_reportFiles); }

    const ReportFile* GetReportFile(std::string_view name_or_file_path_sv, bool search_by_name) const;

    void AddReport(ReportFile report_file);
    void DropReport(const std::string& file_path);


    // resources
    // --------------------------------------------------------------------------
    const std::vector<AppResource>& GetResources() const { return m_resources; }

    const AppResource* GetResource(const std::string& path) const;

    const AppResource& AddResource(AppResource resource);
    void DropResource(const std::string& path);


    // dictionary descriptions
    // --------------------------------------------------------------------------
    const std::vector<DictionaryDescription>& GetDictionaryDescriptions() const { return m_dictionaryDescriptions; }
    std::vector<DictionaryDescription>& GetDictionaryDescriptions()             { return m_dictionaryDescriptions; }

    DictionaryType GetDictionaryType(const CDataDict& dictionary) const;
    const DictionaryDescription* GetDictionaryDescription(const CDataDict& dictionary) const;
    const DictionaryDescription* GetDictionaryDescription(const std::string& dictionary_file_path,
                                                          const std::string& parent_file_path = SO::Empty_string, bool ignore_parent = false) const;
    DictionaryDescription* GetDictionaryDescription(const std::string& dictionary_file_path,
                                                    const std::string& parent_file_path = SO::Empty_string, bool ignore_parent = false);
    const std::string& GetFirstDictionaryFilePathOfType(DictionaryType dictionary_type) const;

    void SetDictionaryDescriptions(std::vector<DictionaryDescription> dictionary_descriptions)    { m_dictionaryDescriptions = std::move(dictionary_descriptions); }
    DictionaryDescription* AddDictionaryDescription(DictionaryDescription dictionary_description) { return &m_dictionaryDescriptions.emplace_back(std::move(dictionary_description)); }
    void DropDictionaryDescription(const std::string& file_path, bool file_path_is_parent);


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

    // All serialization methods (with the exception of WriteJson and serialize) can throw exceptions.
    void Open(InterfaceString file_path, bool silent = false, bool load_text_sources_and_external_application_properties = true);
    void Save(InterfaceString file_path, bool continue_using_file_path = true) const;

    static Application CreateFromJson(const JsonNode& json_node);
    void WriteJson(JsonWriter& json_writer, bool write_to_new_json_object = true) const;

    void serialize(Serializer& ar);


private:
    void CreateFromJsonWorker(const JsonNode& json_node, bool load_text_sources_and_external_application_properties,
                              bool silent, std::shared_ptr<JsonSpecFile::ReaderMessageLogger> message_logger);

    static std::string ConvertPre80SpecFile(InterfaceString file_path);


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

    // Returns false if the name is used by a report.
    bool IsNameUnique(std::string_view name_sv) const;

    static constexpr int SetMinutesVariable(int minutes, int min_minutes = 0, int max_minutes = 360);


private:
    // application properties
    double m_version;
    int m_serializerArchiveVersion;
    EngineAppType m_engineAppType;
    std::string m_name;
    std::string m_label;
    std::string m_applicationFilePath;

    std::string m_applicationPropertiesFilePath;
    std::unique_ptr<ApplicationProperties> m_applicationProperties;

    // other application files
    std::string m_questionTextFilePath;
    std::vector<std::string> m_formFilePaths;
    std::vector<std::string> m_tableSpecFilePaths;
    std::vector<std::string> m_externalDictionaryFilePaths;
    std::vector<CodeFile> m_codeFiles;
    std::vector<AppMessageFile> m_messageFiles;
    std::vector<ReportFile> m_reportFiles;
    std::vector<AppResource> m_resources;

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



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

constexpr int Application::SetMinutesVariable(const int minutes, const int min_minutes/* = 0*/, const int max_minutes/* = 360*/)
{
    return std::max(min_minutes, std::min(minutes, max_minutes));
}
