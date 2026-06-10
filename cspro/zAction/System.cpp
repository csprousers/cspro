#include "stdafx.h"
#include <zToolsO/Hash.h>
#include <zUtilO/PortableFileSystem.h>


CREATE_JSON_KEY(longLabel)


ActionInvoker::Result ActionInvoker::Runtime::System_createShortcut(const JsonNode& json_node, Caller& caller)
{
    const std::string target_file_path = caller.EvaluateAbsolutePath(json_node.Get<std::string>(JK::target));

    if( !PortableFunctions::FileIsRegular(target_file_path) )
        throw CSProException("The shortcut target does not exist: " + target_file_path);

    std::optional<std::string> label = json_node.GetOptional<std::string>(JK::label);

    // when no label is used, use the target's filename
    if( !label.has_value() )
        label = Path::GetFilenameWithoutExtension(target_file_path);

    const std::optional<std::string> long_label = json_node.GetOptional<std::string>(JK::longLabel);

    std::optional<std::string> icon_file_path;

    if( json_node.Contains(JK::icon) )
    {
        icon_file_path = caller.EvaluateAbsolutePath(json_node.Get<std::string>(JK::icon));

        if( !PortableFunctions::FileIsRegular(*icon_file_path) )
            throw CSProException("The shortcut icon does not exist: " + *icon_file_path);
    }

    // the shortcut ID will be a hash of the target and label
    constexpr size_t HashLength = 4;
    const std::string shortcut_id = Hash::Create(target_file_path + *label, HashLength);

    PortableRunner::System_CreateShortcut(shortcut_id, target_file_path, icon_file_path, *label, long_label);

    return Result::Undefined();
}


ActionInvoker::Result ActionInvoker::Runtime::System_getSharableUri(const JsonNode& json_node, Caller& caller)
{
    const auto [paths, return_results_as_an_array] = EvaluateFilePaths(json_node.Get(JK::path), caller, true, true);

    const bool add_write_permission = json_node.Contains(JK::permissions) ?
        ( json_node.GetFromStringOptions(JK::permissions, { "read", "readWrite" }) == 1 ) :
        false;

    std::unique_ptr<JsonStringWriter> json_writer;

    if( return_results_as_an_array )
    {
        json_writer = Json::CreateStringWriter();
        json_writer->BeginArray();
    }

    for( const std::string& path : paths )
    {
        if( !PortableFunctions::FileIsRegular(path) )
            throw FileIO::Exception::FileNotFound(path);

        std::string sharable_uri = PortableFileSystem::CreateSharableUri(path, add_write_permission);

        if( return_results_as_an_array )
        {
            json_writer->Write(sharable_uri);
        }

        else
        {
            ASSERT(paths.size() == 1);
            return Result::String(std::move(sharable_uri));
        }
    }

    ASSERT(return_results_as_an_array);

    json_writer->EndArray();

    return Result::JsonText(*json_writer);
}


ActionInvoker::Result ActionInvoker::Runtime::System_selectDocument(const JsonNode& json_node, Caller& /*caller*/)
{
    const JsonNode content_type_node = json_node.GetOrEmpty(JK::contentType);
    const std::vector<std::string> mime_types = content_type_node.IsArray() ? content_type_node.Get<std::vector<std::string>>() :
                                                content_type_node.IsEmpty() ? std::vector<std::string>({ "*/*" }) :
                                                                              std::vector<std::string>({ content_type_node.Get<std::string>() });

    const bool multiple = json_node.GetOrDefault(JK::multiple, false);

    const std::vector<std::tuple<std::string, std::string>> paths_and_names = PortableRunner::System_ShowSelectDocumentDialog(mime_types, multiple);

    if( paths_and_names.empty() )
        return Result::Undefined();

    ASSERT(multiple || paths_and_names.size() == 1);

    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();

    if( multiple )
        json_writer->BeginArray();

    for( const auto& [path, name] : paths_and_names )
    {
        json_writer->BeginObject()
                    .Write(JK::path, path)
                    .Write(JK::name, name)
                    .EndObject();
    }

    if( multiple )
        json_writer->EndArray();

    return Result::JsonText(*json_writer);
}
