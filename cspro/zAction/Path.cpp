#include "stdafx.h"
#include <zPlatformO/PlatformInterface.h>
#include <zUtilO/CSProExecutables.h>
#include <zUtilO/Interapp.h>
#include <zUtilO/PortableFileSystem.h>
#include <zUtilO/SpecialDirectoryLister.h>
#include <zAppO/PFF.h>


CREATE_JSON_KEY(confirmOverwrite)
CREATE_JSON_KEY(filterDirectories)
CREATE_JSON_KEY(useNativeDialog)
CREATE_JSON_VALUE(directory)
CREATE_JSON_VALUE(file)
CREATE_JSON_VALUE(open)
CREATE_JSON_VALUE(save)


namespace
{
    struct EvaluatedPath;
    void WritePathInfo(JsonWriter& json_writer, const EvaluatedPath& evaluated_path, bool path_is_known_to_exist);
    void WritePathInfo(JsonWriter& json_writer, const std::string& path, bool path_is_known_to_exist);
    std::tuple<EvaluatedPath, std::optional<EvaluatedPath>> EvaluateStartAndRootDirectories(const JsonNode& json_node, ActionInvoker::Caller& caller);


    struct EvaluatedPath
    {
        std::string path_text;
        SpecialDirectoryLister::SpecialDirectory special_directory;

        EvaluatedPath(std::string path_text_)
            :   path_text(std::move(path_text_)),
                special_directory(SpecialDirectoryLister::EvaluateSpecialDirectory(path_text))
        {
            ASSERT(path_text == PortableFunctions::PathRemoveTrailingSlash(path_text));
        }

        EvaluatedPath(const JsonNode& json_node, const char* const key, ActionInvoker::Caller& caller)
            :   path_text(caller.EvaluateAbsolutePath(json_node.Get<std::string>(key), true)),
                special_directory(SpecialDirectoryLister::EvaluateSpecialDirectory(path_text))
        {
            path_text = PortableFunctions::PathToNativeSlash(PortableFunctions::PathRemoveTrailingSlash(path_text));
            ASSERT(!std::holds_alternative<std::string>(special_directory) || std::get<std::string>(special_directory) == path_text);
        }
    };


    void WritePathInfo(JsonWriter& json_writer, const std::string& path, const bool path_is_known_to_exist)
    {
        if( SpecialDirectoryLister::IsSpecialDirectory(path) )
            return WritePathInfo(json_writer, EvaluatedPath(path), path_is_known_to_exist);

        const bool is_file = PortableFunctions::FileIsRegular(path);
        const bool exists = ( path_is_known_to_exist || is_file || PortableFunctions::FileIsDirectory(path) );
        const std::string name = PortableFunctions::PathGetFilename(path);

        json_writer.BeginObject();

        json_writer.WritePath(JK::path, path)
                   .Write(JK::name, name);

        if( is_file )
        {
            const std::string extension = PortableFunctions::PathGetFileExtension(name);

            json_writer.Write(JK::extension, extension)
                       .WriteIfHasValue(JK::contentType, MimeType::GetTypeFromFileExtension(extension));
        }

        json_writer.Write(JK::exists, exists);

        if( exists )
        {
            json_writer.Write(JK::type, is_file ? JV::file : JV::directory)
                       .WriteDate(JK::modifiedTime, PortableFunctions::FileModifiedTime(path));

            if( is_file )
                json_writer.Write(JK::size, PortableFunctions::FileSize(path));
        }

        json_writer.EndObject();
    }


    void WritePathInfo(JsonWriter& json_writer, const EvaluatedPath& evaluated_path, const bool path_is_known_to_exist)
    {
        if( std::holds_alternative<std::string>(evaluated_path.special_directory) )
            return WritePathInfo(json_writer, std::get<std::string>(evaluated_path.special_directory), path_is_known_to_exist);

        const bool exists = ( path_is_known_to_exist || SpecialDirectoryLister::SpecialDirectoryExists(evaluated_path.special_directory, false) );

        json_writer.BeginObject();

        json_writer.Write(JK::path, evaluated_path.path_text)
                   .Write(JK::name, SpecialDirectoryLister::GetSpecialDirectoryName(evaluated_path.special_directory));

        json_writer.Write(JK::exists, exists);

        if( exists )
            json_writer.Write(JK::type, JV::directory);

        json_writer.EndObject();
    }


    std::tuple<EvaluatedPath, std::optional<EvaluatedPath>> EvaluateStartAndRootDirectories(const JsonNode& json_node, ActionInvoker::Caller& caller)
    {
        std::optional<EvaluatedPath> start_directory;
        std::optional<EvaluatedPath> root_directory;

        auto evaluate_directory = [&](std::optional<EvaluatedPath>& evaluated_path, const char* const key)
        {
            evaluated_path.emplace(json_node, key, caller);

            if( !SpecialDirectoryLister::SpecialDirectoryExists(evaluated_path->special_directory, true) )
                throw CSProException("The '%s' is not valid: %s", key, json_node.Get<std::string>(key).c_str());
        };

        if( json_node.Contains(JK::startDirectory) )
            evaluate_directory(start_directory, JK::startDirectory);

        if( json_node.Contains(JK::rootDirectory) )
        {
            evaluate_directory(root_directory, JK::rootDirectory);

            // if a root directory is specified but not a start directory, use the root as the start
            if( !start_directory.has_value() )
            {
                start_directory = root_directory;
            }

            // otherwise make sure the start directory is within the root directory
            else
            {
                SpecialDirectoryLister::ValidateStartAndRootSpecialDirectories(start_directory->special_directory, root_directory->special_directory);
            }
        }

        // if no directory has been provided, use the caller's root directory
        if( !start_directory.has_value() )
        {
            ASSERT(!root_directory.has_value());

            std::string evaluated_root_directory = caller.EvaluateAbsolutePath(".");

            if( !PortableFunctions::FileIsDirectory(evaluated_root_directory) )
                throw CSProException("You must specify a '%s.'", JK::startDirectory);

            start_directory.emplace(std::move(evaluated_root_directory));

        }

        return { std::move(*start_directory), std::move(root_directory) };
    }
}


std::tuple<std::vector<std::string>, bool> ActionInvoker::Runtime::EvaluateFilePaths(const JsonNode& paths_node, ActionInvoker::Caller& caller,
                                                                                     const bool allow_sharable_uris, const bool check_for_file_existence)
{
    std::vector<std::string> paths;
    bool paths_specified_using_array_or_wildcards;

    auto add_path = [&](const JsonNode& path_node)
    {
        std::string path_or_sharable_url = path_node.Get<std::string>();

        if( PortableFileSystem::IsSharableUri(path_or_sharable_url) )
        {
            if( !allow_sharable_uris )
                throw CSProException("You cannot specify a sharable URI: " + path_or_sharable_url);

            paths.emplace_back(std::move(path_or_sharable_url));
        }

        else
        {
            const std::string path = caller.EvaluateAbsolutePath(path_or_sharable_url);

            if( !paths_specified_using_array_or_wildcards )
                paths_specified_using_array_or_wildcards = Path::HasWildcardCharacters(path);

            const size_t initial_paths_size = paths.size();
            DirectoryLister::AddFilePathsWithPossibleWildcard(paths, path, true);
            const size_t paths_added = paths.size() - initial_paths_size;

            // when only one path is added, check that it is a valid path
            // (this is not necessary when multiple paths are added because that means that the paths are true paths resulting from a wildcard)
            if( paths_added == 1 )
            {
                if( check_for_file_existence && !PortableFunctions::FileIsRegular(paths.back()) )
                    throw FileIO::Exception::FileNotFound(paths.back());
            }

            else
            {
                ASSERT(Path::HasWildcardCharacters(path));

                // if no paths were added, make sure that the directory exists
                if( paths_added == 0 )
                {
                    const std::string directory = PortableFunctions::PathGetDirectory(path);

                    if( !PortableFunctions::FileIsDirectory(directory) )
                        throw FileIO::Exception::DirectoryNotFound(directory);
                }
            }
        }
    };

    if( paths_node.IsArray() )
    {
        paths_specified_using_array_or_wildcards = true;

        for( const JsonNode& path_node : paths_node.GetArray() )
            add_path(path_node);
    }

    else
    {
        paths_specified_using_array_or_wildcards = false;

        add_path(paths_node);
    }

    RemoveDuplicateStringsInVectorNoCase(paths);

    return std::make_tuple(std::move(paths), paths_specified_using_array_or_wildcards);
}


ActionInvoker::Result ActionInvoker::Runtime::Path_createDirectory(const JsonNode& json_node, Caller& caller)
{
    std::string path = caller.EvaluateAbsolutePath(json_node.Get<std::string>(JK::path));

    FileIO::CreateDirectories(path);

    return Result::String(std::move(path));
}


ActionInvoker::Result ActionInvoker::Runtime::Path_getPathInfo(const JsonNode& json_node, Caller& caller)
{
    const EvaluatedPath evaluated_path(json_node, JK::path, caller);

    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();

    WritePathInfo(*json_writer, evaluated_path, false);

    return Result::JsonText(*json_writer);
}


ActionInvoker::Result ActionInvoker::Runtime::Path_getDirectoryListing(const JsonNode& json_node, Caller& caller)
{
    const EvaluatedPath evaluated_path(json_node, JK::path, caller);

    const bool recursive = json_node.GetOrDefault(JK::recursive, false);
    bool include_files = true;
    bool include_directories = true;
    constexpr bool include_trailing_slash_on_directories = false;
    const bool filter_directories = json_node.GetOrDefault(JK::filterDirectories, false);

    if( json_node.Contains(JK::type) )
    {
        include_files = ( json_node.GetFromStringOptions(JK::type, { JV::file, JV::directory }) == 0 );
        include_directories = !include_files;
    }

    // create the special directory lister
    std::unique_ptr<SpecialDirectoryLister> special_directory_lister = SpecialDirectoryLister::CreateSpecialDirectoryLister(
        evaluated_path.special_directory, recursive, include_files, include_directories, include_trailing_slash_on_directories, filter_directories);
    ASSERT(special_directory_lister != nullptr);

    const bool detailed_mode = json_node.GetOrDefault(JK::detailed, false);

    if( json_node.Contains(JK::filter) )
        special_directory_lister->SetNameFilter(SpecialDirectoryLister::EvaluateFilter(json_node.Get<std::string_view>(JK::filter)));

    // write the results
    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();

    json_writer->BeginObject();

    json_writer->Write(JK::path, evaluated_path.path_text)
                .WriteIfHasValue(JK::parent, special_directory_lister->GetParentDirectory());

    // JK::paths
    {
        json_writer->BeginArray(JK::paths);

        for( const std::string& path : special_directory_lister->GetSpecialPaths() )
        {
            if( detailed_mode )
            {
                WritePathInfo(*json_writer, path, true);
            }

            else
            {
                json_writer->Write(path);
            }
        }

        json_writer->EndArray();
    }

    json_writer->EndObject();

    return Result::JsonText(*json_writer);
}


ActionInvoker::Result ActionInvoker::Runtime::Path_getSpecialPaths(const JsonNode& /*json_node*/, Caller& /*caller*/)
{
    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();

    json_writer->BeginObject();

    auto write_path = [&](const char* const type, std::string path)
    {
        json_writer->WriteIfNotBlank(type, PortableFunctions::PathRemoveTrailingSlash(std::move(path)));
    };

    const PFF* const pff = GetPff(false);

    if( pff != nullptr )
        write_path("application", PortableFunctions::PathGetDirectory(UTF8_TODO::GetUtf8(pff->GetAppFName())));

#ifdef ANDROID
    write_path("CSEntry", PlatformInterface::GetInstance()->GetCSEntryDirectory());
#endif

    write_path("CSPro", CSProExecutables::GetApplicationDirectory());
    write_path("downloads", GetDownloadsDirectory());
    write_path("html", Html::GetDirectory());
    write_path("temp", GetTempDirectory());

    json_writer->EndObject();

    return Result::JsonText(*json_writer);
}


template<typename CF>
ActionInvoker::Result ActionInvoker::Runtime::ExecutePath_selectFile_showFileDialog(const std::string& base_filename, const JsonNode& json_node, Caller& caller, CF callback_function)
{
    const std::string dialog_file_path = GetHtmlDialogFilePath(base_filename);

    // create the input data
    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();

    json_writer->BeginObject();

    if( json_node.Contains(JK::title) )
        json_writer->Write(JK::title, json_node.Get<std::string_view>(JK::title));

    if( json_node.Contains(JK::filter) )
        json_writer->Write(JK::filter, SpecialDirectoryLister::EvaluateFilter(json_node.Get<std::string_view>(JK::filter)));

    if( json_node.Contains(JK::showDirectories) )
        json_writer->Write(JK::showDirectories, json_node.Get<bool>(JK::showDirectories));

    const auto [start_directory_evaluated_path, root_directory_evaluated_path] = EvaluateStartAndRootDirectories(json_node, caller);

    json_writer->Write(JK::startDirectory, start_directory_evaluated_path.path_text);

    if( root_directory_evaluated_path.has_value() )
        json_writer->Write(JK::rootDirectory, root_directory_evaluated_path->path_text);

    callback_function(*json_writer);

    json_writer->EndObject();

    Result result = ShowHtmlDialog(dialog_file_path, json_writer->ReleaseSharableString());

    try
    {
        if( result.GetType() != Result::Type::Undefined )
        {
            ASSERT(result.GetType() == Result::Type::JsonText);
            return Result::String(PortableFunctions::PathToNativeSlash(Json::Parse(result.GetStringResult().GetString()).Get<std::string>()));
        }
    }
    catch(...) { ASSERT(false); }

    return result;
}


ActionInvoker::Result ActionInvoker::Runtime::Path_selectFile(const JsonNode& json_node, Caller& caller)
{
    return ExecutePath_selectFile_showFileDialog("Path-selectFile.html", json_node, caller, [](const JsonStringWriter& /*json_writer*/) { });
}


ActionInvoker::Result ActionInvoker::Runtime::Path_showFileDialog(const JsonNode& json_node, Caller& caller)
{
    const bool open_file_dialog = ( !json_node.Contains(JK::type) ||
                                    json_node.GetFromStringOptions(JK::type, { JV::open, JV::save }) == 0 );
    const bool confirm_overwrite = json_node.GetOrDefault(JK::confirmOverwrite, true);
    const std::optional<std::string> name = json_node.GetOptional<std::string>(JK::name);

#ifdef WIN_DESKTOP
    if( json_node.GetOrDefault(JK::useNativeDialog, false) )
    {
        const auto [start_directory_evaluated_path, root_directory_evaluated_path] = EvaluateStartAndRootDirectories(json_node, caller);

        if( !SpecialDirectoryLister::IsSpecialDirectory(start_directory_evaluated_path.path_text) )
        {
            std::optional<std::string> filter;

            if( json_node.Contains(JK::filter) )
                filter = SpecialDirectoryLister::EvaluateFilter(json_node.Get<std::string_view>(JK::filter));

            return PortableRunner::Path_ShowNativeFileDialog(start_directory_evaluated_path.path_text, open_file_dialog, confirm_overwrite,
                                                             name, filter, json_node);
        }
    }
#endif

    return ExecutePath_selectFile_showFileDialog("Path-showFileDialog.html", json_node, caller,
        [&](JsonStringWriter& json_writer)
        {
            json_writer.Write(JK::type, open_file_dialog ? JV::open : JV::save)
                       .Write(JK::confirmOverwrite, confirm_overwrite)
                       .WriteIfHasValue(JK::name, name);
        });
}
