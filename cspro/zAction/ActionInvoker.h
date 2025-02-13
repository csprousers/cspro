#pragma once

#include <zAction/zAction.h>
#include <zAction/Caller.h>
#include <zAction/ExceptionWithActionName.h>
#include <zAction/Result.h>
#include <zLogicO/ActionInvoker.h>

class Application;
class BytesToStringConverter;
class Case;
class CDataDict;
class CDEFormFile;
class CommonStore;
class InterpreterAccessor;
template<typename CharType> class JsonNode;
class JsonWriter;
class KeyBasedVirtualFileMappingHandler;
class MessageEvaluator;
class PFF;
class QuestionnaireContentCreator;
class VirtualFileMappingHandler;


namespace ActionInvoker
{
    class Listener;
    class ListenerHolder;

    // the suffix that some executors use for asynchronous action names
    static constexpr std::wstring_view AsyncActionSuffix_sv = _T("Async");


    class ZACTION_API Runtime
    {
        friend BytesToStringConverter;


        // --------------------------------------------------------------------------
        // the general implementation of the runtime;
        // the entry points are all virtual to minimize dependencies on zAction
        // --------------------------------------------------------------------------
    public:
        Runtime();
        virtual ~Runtime();

        Runtime(const Runtime&) = delete;
        Runtime& operator=(const Runtime&) = delete;

        // disables checking access tokens from external callers
        void DisableAccessTokenCheckForExternalCallers();

        // registers an access token
        void RegisterAccessToken(std::wstring access_token);

        // checks if an access token is valid, throwing an exception if not
        virtual void CheckAccessToken(const std::wstring* access_token, Caller& caller);

        // registers a listener, which will exist for the lifetime of the the returned object
        virtual ListenerHolder RegisterListener(std::shared_ptr<Listener> listener);

        // the Process... methods all can throw CSProException exceptions
        virtual Result ProcessExecute(const std::wstring& json_arguments, Caller& caller);
        virtual Result ProcessAction(Action action, const std::optional<std::wstring>& json_arguments, Caller& caller);

        // throws a CSProException with the error
        template<typename... Args>
        [[noreturn]] void IssueError(int message_number, Args... args);

    private:
        // iterates over the listeners (in reverse-added order); return true to continue processing
        template<typename CF>
        void IterateOverListeners(CF callback_function);

        JsonNode<wchar_t> ParseJson(const std::wstring& json_arguments, const Action* action);

        Action GetActionFromJson(const JsonNode<wchar_t>& json_node);

        Result RunFunction(Action action, const JsonNode<wchar_t>& json_node, Caller& caller);

        // convenience methods related to the ObjectTransporter
        InterpreterAccessor& GetInterpreterAccessor();

    private:
        std::unique_ptr<std::set<std::wstring>> m_registeredAccessTokensForExternalCallers; // null if access tokens are not required

        std::shared_ptr<std::vector<Listener*>> m_listeners;

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
                   std::shared_ptr<const CDataDict>> GetApplicationComponents(const std::optional<wstring_view>& name_sv);


        // Data
        Result GetQuestionnaireContentWithCaseData(QuestionnaireContentCreator& questionnaire_content_creator, std::unique_ptr<Case> data_case,
                                                   const JsonNode<wchar_t>& json_node, bool write_all_content, bool case_content_is_from_current_case);


        // Localhost
        const std::wstring& CreateVirtualFileAndGetUrl(std::unique_ptr<VirtualFileMappingHandler> virtual_file_mapping_handler,
                                                       NullTerminatedString mapping_filename = _T(""));


        // Logic
        template<typename CF>
        Result Logic_executeWorker(CF callback_function);

        template<typename SJO>
        Result Logic_getSymbolWorker(const JsonNode<wchar_t>& json_node, SJO symbol_json_output);


        // Message
        std::wstring GetMessageText(const JsonNode<wchar_t>& json_node, Action action);


        // Path
        std::tuple<std::vector<std::wstring>, bool> EvaluateFilePaths(const JsonNode<wchar_t>& paths_node, Caller& caller, bool allow_sharable_uris);

        template<typename CF>
        Result ExecutePath_selectFile_showFileDialog(const std::wstring& base_filename, const JsonNode<wchar_t>& json_node, Caller& caller, CF callback_function);


        // Settings
        CommonStore& SwitchToProperSettingsTable(const JsonNode<wchar_t>& json_node, bool& using_UserSettings_table);


        // UserInterface
        std::wstring GetHtmlDialogFilename(const std::wstring& base_filename);
        Result ShowHtmlDialog(const std::wstring& dialog_path, std::wstring input_data, std::optional<std::wstring> display_options = std::nullopt);

    private:
        std::vector<std::unique_ptr<VirtualFileMappingHandler>> m_localHostVirtualFileMappingHandlers;
        std::vector<std::shared_ptr<KeyBasedVirtualFileMappingHandler>> m_localHostKeyBasedVirtualFileMappingHandlers;

        std::unique_ptr<MessageEvaluator> m_messageEvaluator;

        class SqliteDbWrapper;
        std::map<int, std::shared_ptr<SqliteDbWrapper>> m_sqliteDbWrappers;


        // --------------------------------------------------------------------------
        // the functions for each action
        // --------------------------------------------------------------------------
        using ActionFunctionPointer = Result (Runtime::*)(const JsonNode<wchar_t>&, Caller&);
        static const std::map<Action, ActionFunctionPointer> m_functions;

        // --- CS_AUTOGENERATED_START -----------------------------------------------

        Result execute(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result registerAccessToken(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result Application_getFormFile(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result Application_getQuestionnaireContent(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result Application_getQuestionText(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result Clipboard_getText(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result Clipboard_putText(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result Data_getCase(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result Dictionary_getDictionary(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result File_copy(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result File_readBytes(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result File_readLines(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result File_readText(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result File_writeBytes(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result File_writeLines(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result File_writeText(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result Hash_createHash(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result Hash_createMd5(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result Localhost_mapActionResult(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result Localhost_mapFile(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result Localhost_mapSymbol(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result Localhost_mapText(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result Logic_eval(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result Logic_getSymbol(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result Logic_getSymbolMetadata(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result Logic_getSymbolValue(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result Logic_invoke(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result Logic_updateSymbolValue(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result Message_formatText(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result Message_getText(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result Path_createDirectory(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result Path_getDirectoryListing(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result Path_getPathInfo(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result Path_getSpecialPaths(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result Path_selectFile(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result Path_showFileDialog(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result Settings_getValue(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result Settings_putValue(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result Sqlite_close(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result Sqlite_exec(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result Sqlite_open(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result Sqlite_rekey(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result System_getSharableUri(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result System_selectDocument(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result UI_alert(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result UI_closeDialog(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result UI_enumerateWebViews(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result UI_getDisplayOptions(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result UI_getInputData(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result UI_getMaxDisplayDimensions(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result UI_postWebMessage(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result UI_setDisplayOptions(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result UI_showDialog(const JsonNode<wchar_t>& json_node, Caller& caller);
        Result UI_view(const JsonNode<wchar_t>& json_node, Caller& caller);

        // --- CS_AUTOGENERATED_END -------------------------------------------------
    };
}
