#pragma once

#include <zAction/zAction.h>
#include <zAction/Caller.h>
#include <zAction/Exception.h>
#include <zAction/Result.h>
#include <zLogicO/ActionInvoker.h>

class Application;
class BytesToStringConverter;
class Case;
class CDataDict;
class CDEFormFile;
class CommonStore;
class DataRepository;
class InterpreterAccessor;
class JsonReaderInterface;
class KeyBasedVirtualFileMappingHandler;
class MessageEvaluator;
class PFF;
class QuestionnaireContentCreator;
class StringToBytesConverter;
class VirtualFileMappingHandler;


namespace ActionInvoker
{
    enum class DataQueryContentType;
    class Listener;
    class ListenerHolder;
    class Runtime;

    struct CachedBinaryContent
    {
        std::shared_ptr<const std::vector<std::byte>> bytes;
        std::string mime_type;
    };

    // the suffix that some executors use for asynchronous action names
    constexpr std::string_view AsyncActionSuffix_sv = "Async";
}


class ZACTION_API ActionInvoker::Runtime
{
    friend BytesToStringConverter;
    friend StringToBytesConverter;

    // --------------------------------------------------------------------------
    // This is the general implementation of the runtime.
    // The entry points are all virtual to minimize dependencies on zAction.
    // --------------------------------------------------------------------------
public:
    Runtime();
    virtual ~Runtime();

    Runtime(const Runtime&) = delete;
    Runtime& operator=(const Runtime&) = delete;

    // Disables checking access tokens from external callers.
    void DisableAccessTokenCheckForExternalCallers();

    // Registers an access token.
    void RegisterAccessToken(std::string access_token);

    // Checks if an access token is valid, throwing an exception if not.
    virtual void CheckAccessToken(const std::string* access_token, Caller& caller);

    // Registers a listener, which will exist for the lifetime of the the returned object.
    virtual ListenerHolder RegisterListener(std::shared_ptr<Listener> listener);

    // The Process... methods all can throw CSProException exceptions:
    virtual Result ProcessExecute(const std::string& json_arguments, Caller& caller);
    virtual Result ProcessAction(Action action, const SharableString& json_arguments, Caller& caller);

    // Returns cached binary content, and the optional MIME type, for a cspro:///cache/... URI,
    // throwing an exception if not present.
    virtual CachedBinaryContent GetCachedBinaryContent(std::string_view cache_uri_sv);

    // Throws a CSProException with the error.
    template<typename... Args>
    [[noreturn]] void IssueError(int message_number, Args const&... args);

    // Iterates over the listeners (in reverse-added order); return true to continue processing.
    template<typename CF>
    void IterateOverListeners(CF callback_function);

    // Iterates over the listeners (as above) but only executes the callback when the listener's caller ID
    // matches the supplied caller's ID.
    template<typename CF>
    void IterateOverListeners(const Caller& caller, CF callback_function);

private:
    JsonNode ParseJson(std::string_view json_arguments_sv, Caller& caller, const Action* action);

    Action GetActionFromJson(const JsonNode& json_node);

    Result RunFunction(Action action, const JsonNode& json_node, Caller& caller);

    // Creates a unique resource ID and associates it with the caller so that it can be
    // accessed in future calls without requiring the explicit specification of a resource ID.
    enum class Resource { Data, FetchBody, SqliteDb, SyncService};
    int CreateResourceId(Resource resource, Caller& caller);

    // Returns the resource ID, calculated implicitly (if only one resource for the type exists for the caller),
    // or explicitly (if specified as part of the arguments).
    int GetResourceId(Resource resource, const JsonNode& json_node, Caller& caller, const char* id_key,
                      const char* not_specified_formatter, const char* multiple_implicit_formatter) const;

    // Removes any references to the the resource ID.
    void DestroyResourceId(int resource_id);

    // Convenience methods related to the ObjectTransporter:
    InterpreterAccessor& GetInterpreterAccessor();

private:
    std::unique_ptr<JsonReaderInterface> m_exceptionThrowingJsonReaderInterface;

    std::unique_ptr<std::set<std::string>> m_registeredAccessTokensForExternalCallers; // null if access tokens are not required

    std::shared_ptr<std::vector<Listener*>> m_listeners;

    std::shared_ptr<Caller*> m_currentCaller;

    std::map<int, std::vector<std::tuple<Resource, int>>> m_resourceIdCallerMap; // caller ID -> resource type and ID

    std::tuple<size_t, std::shared_ptr<InterpreterAccessor>> m_interpreterAccessor;


    // --------------------------------------------------------------------------
    // everything below is related to action processing
    // --------------------------------------------------------------------------
private:
    // Application
    const Application* GetApplication(bool throw_exception_if_does_not_exist);
    const PFF* GetPff(bool throw_exception_if_does_not_exist);

    template<typename RequiredComponentType>
    std::tuple<const Application*,
               std::shared_ptr<const CDEFormFile>,
               std::shared_ptr<const CDataDict>> GetApplicationComponents(std::optional<std::string_view> name_sv);


    // Data
    static std::unique_ptr<Case> ReadCase(const JsonNode& json_node, DataRepository& data_repository,
                                          bool return_null_if_no_case_identifier_present);

    Result GetQuestionnaireContentWithCaseData(std::variant<std::shared_ptr<const CDataDict>, std::unique_ptr<QuestionnaireContentCreator>> dictionary_or_questionnaire_content_creator,
                                               std::unique_ptr<const Case> data_case, const JsonNode& json_node,
                                               bool write_all_content, bool case_content_is_from_current_case);

    Result QueryDataRepository(const JsonNode& json_node, Caller& caller, std::optional<DataQueryContentType> query_type);

    // File
    static FileOverwriteFlag EvaluateFileOverwriteFlag(const JsonNode& json_node);


    // Localhost
    SharableString CreateVirtualFileAndGetUrl(std::unique_ptr<VirtualFileMappingHandler> virtual_file_mapping_handler,
                                              cs::string_sz mapping_filename = "");


    // Logic
    template<typename CF>
    Result Logic_executeWorker(CF callback_function);

    template<typename SJO>
    Result Logic_getSymbolWorker(const JsonNode& json_node, SJO symbol_json_output);


    // Message
    SharableString GetMessageText(const JsonNode& json_node, Action action);


    // Path
    std::tuple<std::vector<std::string>, bool> EvaluateFilePaths(const JsonNode& paths_node, Caller& caller,
                                                                 bool allow_sharable_uris, bool check_for_file_existence);

    template<typename CF>
    Result ExecutePath_selectFile_showFileDialog(const std::string& base_filename, const JsonNode& json_node, Caller& caller, CF callback_function);


    // Settings
    std::shared_ptr<CommonStore> SwitchToProperSettingsTable(const JsonNode& json_node, bool& using_UserSettings_table);


    // UserInterface
    std::string GetHtmlDialogFilePath(const std::string& base_filename);
    Result ShowHtmlDialog(const std::string& dialog_file_path, SharableString input_data, SharableString display_options = SharableString());


private:
    std::map<std::string, CachedBinaryContent> m_cachedBinaryContent;

    std::vector<std::unique_ptr<VirtualFileMappingHandler>> m_localHostVirtualFileMappingHandlers;
    std::vector<std::shared_ptr<KeyBasedVirtualFileMappingHandler>> m_localHostKeyBasedVirtualFileMappingHandlers;

    class DataWrapper;
    std::map<int, std::shared_ptr<DataWrapper>> m_dataWrappers;

    class FetchWrapper;
    std::map<int, std::shared_ptr<FetchWrapper>> m_fetchWrappers;

    std::unique_ptr<MessageEvaluator> m_messageEvaluator;

    class SqliteDbWrapper;
    std::map<int, std::shared_ptr<SqliteDbWrapper>> m_sqliteDbWrappers;

    class SyncServiceWrapper;
    std::map<int, std::shared_ptr<SyncServiceWrapper>> m_syncServiceWrappers;


    // --------------------------------------------------------------------------
    // the functions for each action
    // --------------------------------------------------------------------------
    using ActionFunctionPointer = Result (Runtime::*)(const JsonNode&, Caller&);
    static const std::map<Action, ActionFunctionPointer> m_functions;

    // --- CS_AUTOGENERATED_START -----------------------------------------------

    Result execute(const JsonNode& json_node, Caller& caller);
    Result registerAccessToken(const JsonNode& json_node, Caller& caller);
    Result throwException(const JsonNode& json_node, Caller& caller);
    Result Application_getFormFile(const JsonNode& json_node, Caller& caller);
    Result Application_getQuestionnaireContent(const JsonNode& json_node, Caller& caller);
    Result Application_getQuestionText(const JsonNode& json_node, Caller& caller);
    Result Clipboard_getText(const JsonNode& json_node, Caller& caller);
    Result Clipboard_putText(const JsonNode& json_node, Caller& caller);
    Result Data_close(const JsonNode& json_node, Caller& caller);
    Result Data_contains(const JsonNode& json_node, Caller& caller);
    Result Data_countCases(const JsonNode& json_node, Caller& caller);
    Result Data_deleteCase(const JsonNode& json_node, Caller& caller);
    Result Data_getCase(const JsonNode& json_node, Caller& caller);
    Result Data_getCurrentCase(const JsonNode& json_node, Caller& caller);
    Result Data_open(const JsonNode& json_node, Caller& caller);
    Result Data_query(const JsonNode& json_node, Caller& caller);
    Result Data_queryCases(const JsonNode& json_node, Caller& caller);
    Result Data_queryKeys(const JsonNode& json_node, Caller& caller);
    Result Data_readCase(const JsonNode& json_node, Caller& caller);
    Result Data_writeCase(const JsonNode& json_node, Caller& caller);
    Result Dictionary_getDictionary(const JsonNode& json_node, Caller& caller);
    Result File_copy(const JsonNode& json_node, Caller& caller);
    Result File_readBytes(const JsonNode& json_node, Caller& caller);
    Result File_readLines(const JsonNode& json_node, Caller& caller);
    Result File_readText(const JsonNode& json_node, Caller& caller);
    Result File_writeBytes(const JsonNode& json_node, Caller& caller);
    Result File_writeLines(const JsonNode& json_node, Caller& caller);
    Result File_writeText(const JsonNode& json_node, Caller& caller);
    Result Hash_createHash(const JsonNode& json_node, Caller& caller);
    Result Hash_createMd5(const JsonNode& json_node, Caller& caller);
    Result Localhost_mapActionResult(const JsonNode& json_node, Caller& caller);
    Result Localhost_mapFile(const JsonNode& json_node, Caller& caller);
    Result Localhost_mapSymbol(const JsonNode& json_node, Caller& caller);
    Result Localhost_mapText(const JsonNode& json_node, Caller& caller);
    Result Logic_eval(const JsonNode& json_node, Caller& caller);
    Result Logic_getSymbol(const JsonNode& json_node, Caller& caller);
    Result Logic_getSymbolMetadata(const JsonNode& json_node, Caller& caller);
    Result Logic_getSymbolValue(const JsonNode& json_node, Caller& caller);
    Result Logic_invoke(const JsonNode& json_node, Caller& caller);
    Result Logic_setSymbolValue(const JsonNode& json_node, Caller& caller);
    Result Logic_updateSymbolValue(const JsonNode& json_node, Caller& caller);
    Result Message_formatText(const JsonNode& json_node, Caller& caller);
    Result Message_getText(const JsonNode& json_node, Caller& caller);
    Result Network_fetch(const JsonNode& json_node, Caller& caller);
    Result Network_fetchBody(const JsonNode& json_node, Caller& caller);
    Result Network_fetchBytes(const JsonNode& json_node, Caller& caller);
    Result Network_fetchFile(const JsonNode& json_node, Caller& caller);
    Result Network_fetchJson(const JsonNode& json_node, Caller& caller);
    Result Network_fetchText(const JsonNode& json_node, Caller& caller);
    Result Path_createDirectory(const JsonNode& json_node, Caller& caller);
    Result Path_getDirectoryListing(const JsonNode& json_node, Caller& caller);
    Result Path_getPathInfo(const JsonNode& json_node, Caller& caller);
    Result Path_getSpecialPaths(const JsonNode& json_node, Caller& caller);
    Result Path_selectFile(const JsonNode& json_node, Caller& caller);
    Result Path_showFileDialog(const JsonNode& json_node, Caller& caller);
    Result Settings_getValue(const JsonNode& json_node, Caller& caller);
    Result Settings_putValue(const JsonNode& json_node, Caller& caller);
    Result Sqlite_close(const JsonNode& json_node, Caller& caller);
    Result Sqlite_exec(const JsonNode& json_node, Caller& caller);
    Result Sqlite_open(const JsonNode& json_node, Caller& caller);
    Result Sqlite_rekey(const JsonNode& json_node, Caller& caller);
    Result Sync_connect(const JsonNode& json_node, Caller& caller);
    Result Sync_disconnect(const JsonNode& json_node, Caller& caller);
    Result Sync_sendMessage(const JsonNode& json_node, Caller& caller);
    Result Sync_syncData(const JsonNode& json_node, Caller& caller);
    Result Sync_syncParadata(const JsonNode& json_node, Caller& caller);
    Result System_createShortcut(const JsonNode& json_node, Caller& caller);
    Result System_getSharableUri(const JsonNode& json_node, Caller& caller);
    Result System_selectDocument(const JsonNode& json_node, Caller& caller);
    Result UI_alert(const JsonNode& json_node, Caller& caller);
    Result UI_close(const JsonNode& json_node, Caller& caller);
    Result UI_closeDialog(const JsonNode& json_node, Caller& caller);
    Result UI_enumerateWebViews(const JsonNode& json_node, Caller& caller);
    Result UI_getDisplayOptions(const JsonNode& json_node, Caller& caller);
    Result UI_getInputData(const JsonNode& json_node, Caller& caller);
    Result UI_getMaxDisplayDimensions(const JsonNode& json_node, Caller& caller);
    Result UI_postWebMessage(const JsonNode& json_node, Caller& caller);
    Result UI_setDisplayOptions(const JsonNode& json_node, Caller& caller);
    Result UI_setWebViewOptions(const JsonNode& json_node, Caller& caller);
    Result UI_showDialog(const JsonNode& json_node, Caller& caller);
    Result UI_view(const JsonNode& json_node, Caller& caller);

    // --- CS_AUTOGENERATED_END -------------------------------------------------
};
