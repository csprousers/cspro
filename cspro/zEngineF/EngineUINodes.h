#pragma once

class IMapUI;
class MappingProperties;
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
        SharableString overlay_message; // nullable
        std::string output_file_path;
    };


    struct CreateMapUINode
    {
        std::unique_ptr<IMapUI>& map_ui;
        cs::non_null_shared_or_raw_ptr<const MappingProperties> mapping_properties;
    };


    struct ColorizeLogicNode
    {
        const std::string& logic;
        std::string html;
    };


    struct CreateVirtualFileMappingAroundViewHtmlContentNode
    {
        SharableString html;
        const std::string& local_file_server_root_directory;
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
        SharableString package_name;
        SharableString activity_name;
        std::string evaluated_call;
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
