#include "stdafx.h"
#include <zUtilO/PortableFileSystem.h>


ActionInvoker::Result ActionInvoker::Runtime::System_getSharableUri(const JsonNode<wchar_t>& json_node, Caller& caller)
{
    const auto [paths, return_results_as_an_array] = EvaluateFilePaths(json_node.Get(JK::path), caller, true);

    const bool add_write_permission = json_node.Contains(JK::permissions) ?
        ( json_node.GetFromStringOptions(JK::permissions, std::initializer_list<const TCHAR*>({_T("read"), _T("readWrite") })) == 1 ) :
        false;

    std::unique_ptr<JsonStringWriter<wchar_t>> json_writer;

    if( return_results_as_an_array )
    {
        json_writer = Json::CreateStringWriter();
        json_writer->BeginArray();
    }

    for( const std::wstring& path : paths )
    {
        if( !PortableFunctions::FileIsRegular(path) )
            throw FileIO::Exception::FileNotFound(path);

        std::wstring sharable_uri = PortableFileSystem::CreateSharableUri(path, add_write_permission);

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

    return Result::JsonText(json_writer);
}


ActionInvoker::Result ActionInvoker::Runtime::System_selectDocument(const JsonNode<wchar_t>& json_node, Caller& /*caller*/)
{
    const auto& content_type_node = json_node.GetOrEmpty(JK::contentType);
    const std::vector<std::wstring> mime_types = content_type_node.IsArray() ? content_type_node.Get<std::vector<std::wstring>>() :
                                                 content_type_node.IsEmpty() ? std::vector<std::wstring>({ _T("*/*") }) :
                                                                               std::vector<std::wstring>({ content_type_node.Get<std::wstring>() });    

    const bool multiple = json_node.GetOrDefault(JK::multiple, false);

    const std::vector<std::tuple<std::wstring, std::wstring>> paths_and_names = PortableRunner::SystemShowSelectDocumentDialog(mime_types, multiple);

    if( paths_and_names.empty() )
        return Result::Undefined();

    ASSERT(multiple || paths_and_names.size() == 1);

    auto json_writer = Json::CreateStringWriter();

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

    return Result::JsonText(json_writer);
}
