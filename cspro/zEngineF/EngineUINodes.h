#pragma once

class PFF;
class PffExecutor;
class SystemApp;
class VirtualFileMapping;


namespace EngineUI
{
    enum class Type
    {
        CaptureImage,
        ColorizeLogic,
        CreateMapUI,
        CreateUserbar,
        CreateVirtualFileMappingAroundViewHtmlContent,
        EditNote,
        ExecSystemApp,
        HtmlDialogsDirectoryQuery,
        Prompt,
        RunPffExecutor,
        View,
    };


    struct CaptureImageNode
    {
        enum class Action { TakePhoto, CaptureSignature };
        Action action;
        std::optional<std::wstring> overlay_message;
        std::wstring output_filename;
    };


    struct ColorizeLogicNode
    {
        const std::wstring& logic;
        std::wstring html;
    };


    struct CreateVirtualFileMappingAroundViewHtmlContentNode
    {
        std::string html;
        const std::wstring& local_file_server_root_directory;
        std::unique_ptr<VirtualFileMapping> virtual_file_mapping;
    };


    struct EditNoteNode
    {
        CString& note;
        const CString& title;
        bool case_note;
    };


    struct ExecSystemAppNode
    {
        SystemApp& system_app;
        std::wstring evaluated_call;
        std::wstring package_name;
        std::optional<std::wstring> activity_name;
        std::function<bool()> function_to_run_in_engine_thread;
    };


    struct PromptNode
    {
        CString title;
        CString initial_value;
        CString return_value;
        bool numeric;
        bool password;
        bool upper_case;
        bool multiline;
    };


    struct RunPffExecutorNode
    {
        const PFF& pff;
        std::shared_ptr<PffExecutor> pff_executor;
        std::exception_ptr thrown_exception;
    };
}
