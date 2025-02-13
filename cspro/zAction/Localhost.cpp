#include "stdafx.h"
#include <zHtml/PortableLocalhost.h>
#include <zHtml/FileSystemVirtualFileMappingHandler.h>


CREATE_JSON_KEY(evaluateImmediately)


const std::wstring& ActionInvoker::Runtime::CreateVirtualFileAndGetUrl(std::unique_ptr<VirtualFileMappingHandler> virtual_file_mapping_handler,
                                                                       NullTerminatedString mapping_filename/* = _T("")*/)
{
    ASSERT(virtual_file_mapping_handler != nullptr);

    PortableLocalhost::CreateVirtualFile(*virtual_file_mapping_handler, mapping_filename);
    const std::wstring& url = virtual_file_mapping_handler->GetUrl();

    m_localHostVirtualFileMappingHandlers.emplace_back(std::move(virtual_file_mapping_handler));

    return url;
}


ActionInvoker::Result ActionInvoker::Runtime::Localhost_mapActionResult(const JsonNode<wchar_t>& json_node, Caller& caller)
{
    Result result = execute(json_node, caller);
    std::unique_ptr<VirtualFileMappingHandler> virtual_file_mapping_handler;

    if( result.GetType() == Result::Type::Undefined )
    {
        virtual_file_mapping_handler = std::make_unique<FourZeroFourVirtualFileMappingHandler>();
    }

    else if( result.GetType() == Result::Type::JsonText )
    {
        virtual_file_mapping_handler = std::make_unique<TextVirtualFileMappingHandler>(result.ReleaseStringResult(), MimeType::Type::Json);
    }

    else
    {
        ASSERT(result.GetType() == Result::Type::Bool ||
               result.GetType() == Result::Type::Number || 
               result.GetType() == Result::Type::String);

        virtual_file_mapping_handler = std::make_unique<TextVirtualFileMappingHandler>(result.ReleaseResultAsString<false>());
    }

    const std::wstring& url = CreateVirtualFileAndGetUrl(std::move(virtual_file_mapping_handler));

    return Result::String(url);
}


ActionInvoker::Result ActionInvoker::Runtime::Localhost_mapFile(const JsonNode<wchar_t>& json_node, Caller& caller)
{
    const std::wstring path = caller.EvaluateAbsolutePath(json_node.Get<std::wstring>(JK::path));
    std::optional<std::wstring> content_type_override = json_node.GetOptional<std::wstring>(JK::contentType);
    const bool evaluate_immediately = json_node.GetOrDefault(JK::evaluateImmediately, false);
    std::optional<std::wstring> path_override;

    if( !PortableFunctions::FileIsRegular(path) )
        throw FileIO::Exception::FileNotFound(path);

    if( json_node.Contains(JK::pathOverride) )
    {
        path_override = caller.EvaluateAbsolutePath(json_node.Get<std::wstring>(JK::pathOverride));

        const std::wstring directory = PortableFunctions::PathGetDirectory(*path_override);

        if( !PortableFunctions::FileIsDirectory(directory) )
            throw CSProException(_T("You cannot map a file to a directory that does not exist: ") + directory);

        if( PortableFunctions::FileIsDirectory(*path_override) )
            throw CSProException(_T("You cannot map a file to a directory: ") + *path_override);
    }

    // with no special parameters, simply return a filename URL
    if( !evaluate_immediately && !path_override.has_value() )
        return Result::String(PortableLocalhost::CreateFilenameUrl(path));

    std::unique_ptr<VirtualFileMappingHandler> virtual_file_mapping_handler;

    if( !content_type_override.has_value() )
        content_type_override = MimeType::GetServerTypeFromFileExtension(PortableFunctions::PathGetFileExtension(path));

    // when evaluating immediately, read the file and store the content in a DataVirtualFileMappingHandler
    if( evaluate_immediately )
    {
        std::unique_ptr<std::vector<std::byte>> content = FileIO::Read(path);
        virtual_file_mapping_handler = std::make_unique<DataVirtualFileMappingHandler<const std::vector<std::byte>>>(std::move(*content), ValueOrDefault(std::move(content_type_override)));
    }

    // otherwise use FileVirtualFileMappingHandler to serve the file
    else
    {
        virtual_file_mapping_handler = std::make_unique<FileVirtualFileMappingHandler>(path, false, ValueOrDefault(std::move(content_type_override)));
    }

    // wrap the handler in a file system virtual file mapping handler
    const std::wstring& path_for_special_handler = path_override.has_value() ? *path_override : path;

    auto file_system_virtual_file_mapping_handler = std::make_unique<FileSystemVirtualFileMappingHandler>(path_for_special_handler,
                                                                                                          std::move(virtual_file_mapping_handler));
    PortableLocalhost::CreateVirtualDirectory(*file_system_virtual_file_mapping_handler);

    std::wstring url = file_system_virtual_file_mapping_handler->CreateUrlForPath(path_for_special_handler);

    m_localHostKeyBasedVirtualFileMappingHandlers.emplace_back(std::move(file_system_virtual_file_mapping_handler));

    return Result::String(std::move(url));
}


ActionInvoker::Result ActionInvoker::Runtime::Localhost_mapSymbol(const JsonNode<wchar_t>& json_node, Caller& /*caller*/)
{
    const std::wstring symbol_name = json_node.Get<std::wstring>(JK::name);
    std::optional<std::wstring> content_type_override = json_node.GetOptional<std::wstring>(JK::contentType);
    const bool evaluate_immediately = json_node.GetOrDefault(JK::evaluateImmediately, false);

    std::wstring url = GetInterpreterAccessor().LocalhostCreateMappingForBinarySymbol(symbol_name, std::move(content_type_override), evaluate_immediately);

    return Result::String(std::move(url));
}


ActionInvoker::Result ActionInvoker::Runtime::Localhost_mapText(const JsonNode<wchar_t>& json_node, Caller& /*caller*/)
{
    const wstring_view text_sv = json_node.Get<wstring_view>(JK::text);
    const std::wstring content_type_override = json_node.Contains(JK::contentType) ? json_node.Get<std::wstring>(JK::contentType) :
                                                                                     MimeType::ServerType::TextUtf8;

    auto virtual_file_mapping_handler = std::make_unique<TextVirtualFileMappingHandler>(text_sv, content_type_override);

    const std::wstring& url = CreateVirtualFileAndGetUrl(std::move(virtual_file_mapping_handler));

    return Result::String(url);
}
