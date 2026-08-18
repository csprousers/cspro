#include "stdafx.h"
#include "IncludesRT.h"
#include "AllSymbols.h"
#include "EngineAccessor.h"
#include "EngineCaseConstructionReporter.h"
#include "Nodes/File.h"
#include "Nodes/Path.h"
#include <engine/DicX.h>
#include <engine/Nodes.h>
#include <zUtilO/CSProExecutables.h>
#include <zUtilO/PathHelpers.h>
#include <zUtilO/PortableFileSystem.h>
#include <zUtilO/SpecialDirectoryLister.h>
#include <zUtilF/SelectFileDlg.h>
#include <zParadataO/Logger.h>
#include <zConcatO/Concatenator.h>
#include <zConcatO/ConcatenatorReporter.h>


// --------------------------------------------------------------------------
// path routines
// --------------------------------------------------------------------------

const std::string& LogicInterpreter::GetCurrentApplicationFilePath()
{
    return ( m_engineData->pff != nullptr ) ? UTF8_TODO::Create_Reference(m_engineData->pff->GetAppFName()) :
                                              ReturnProgrammingError(SO::Empty_string);
}


const std::string& LogicInterpreter::GetCurrentWorkingDirectory()
{
    if( m_currentWorkingDirectory.empty() )
    {
        m_currentWorkingDirectory = ( m_engineData->pff != nullptr ) ? GetWorkingDirectory(GetCurrentApplicationFilePath()) :
                                                                       GetWorkingDirectory();
    }

    return m_currentWorkingDirectory;
}


std::string LogicInterpreter::GetAbsolutePath(std::string path)
{
    return MakeFullPath(GetCurrentWorkingDirectory(), std::move(path));
}


void LogicInterpreter::MakeAbsolutePath(std::string& path)
{
    path = GetAbsolutePath(std::move(path));
}


std::string LogicInterpreter::EvaluatePath(const int program_index)
{
    return GetAbsolutePath(Evaluate<std::string>(program_index));
}


SharableString LogicInterpreter::EvaluatePathOrUrl(const int program_index)
{
    SharableString path_or_url = Evaluate<SharableString>(program_index);

    if( !Encoders::IsDataOrHttpUrl(*path_or_url) )
        MakeAbsolutePath(path_or_url.MakeModifiable());

    return path_or_url;
}


void LogicInterpreter::MakeAbsolutePath(ConnectionString& connection_string)
{
    connection_string.AdjustRelativePath(GetCurrentWorkingDirectory());
}


ConnectionString LogicInterpreter::EvaluateConnectionString(const int program_index)
{
    ConnectionString connection_string(Evaluate<SharableString>(program_index).GetString());
    MakeAbsolutePath(connection_string);
    return connection_string;
}



// --------------------------------------------------------------------------
// path functions
// --------------------------------------------------------------------------

namespace PathRT
{
    constexpr std::string_view InvalidPathText_sv = "<invalid path>";

    template<bool AllowSpecialDirectories, typename T,
             typename RV = typename std::conditional<AllowSpecialDirectories, SpecialDirectoryLister::SpecialDirectory, SharableString>::type>
    RV Get_DVE_RV(T value); //Get_DVE_RV = GetDirectoryVariantEvaluatorReturnValue

    template<bool AllowSpecialDirectories,
             typename RV = typename std::conditional<AllowSpecialDirectories, SpecialDirectoryLister::SpecialDirectory, SharableString>::type>
    RV DirectoryVariantEvaluator(LogicInterpreter& interpreter, int directory_variant_index);

    SharableString PathFilterEvaluator(LogicInterpreter& interpreter, int filter_type_or_expression);
}


template<bool AllowSpecialDirectories, typename T,
         typename RV/* = typename std::conditional<AllowSpecialDirectories, SpecialDirectoryLister::SpecialDirectory, SharableString>::type*/>
RV PathRT::Get_DVE_RV(T value)
{
    if constexpr(AllowSpecialDirectories && std::is_same_v<T, SharableString>)
    {
        return SpecialDirectoryLister::SpecialDirectory(value.Release());
    }

    else
    {
        return value;
    }
}


template<bool AllowSpecialDirectories,
         typename RV/* = typename std::conditional<AllowSpecialDirectories, SpecialDirectoryLister::SpecialDirectory, SharableString>::type*/>
RV PathRT::DirectoryVariantEvaluator(LogicInterpreter& interpreter, const int directory_variant_index)
{
    const auto& directory_variant_node = interpreter.GetNode<Nodes::DirectoryVariant>(directory_variant_index);

    auto GetSymbolTable = [&]() -> const Logic::SymbolTable& { return interpreter.GetSymbolTable(); };


    // --------------------------------------------------------------------------
    // string expressions
    // --------------------------------------------------------------------------
    if( directory_variant_node.type == Nodes::DirectoryVariant::Type::String )
    {
        SharableString filename_or_path = interpreter.Evaluate<SharableString>(directory_variant_node.code_or_expression);

        if constexpr(AllowSpecialDirectories)
        {
            if( SpecialDirectoryLister::IsSpecialDirectory(*filename_or_path) )
                return SpecialDirectoryLister::EvaluateSpecialDirectory(*filename_or_path);
        }

        interpreter.MakeAbsolutePath(filename_or_path.MakeModifiable());

        return Get_DVE_RV<AllowSpecialDirectories>(std::move(filename_or_path));
    }


    // --------------------------------------------------------------------------
    // symbols
    // --------------------------------------------------------------------------
    if( directory_variant_node.type == Nodes::DirectoryVariant::Type::Symbol )
    {
        const Symbol& symbol = NPT_Ref(directory_variant_node.code_or_expression);
        std::string symbol_file_path;

        if( symbol.IsA(SymbolType::Dictionary) )
        {
#ifdef WIN_DESKTOP
            symbol_file_path = assert_cast<const EngineDictionary&>(symbol).GetDictionary().GetFilePath();
#else
            // 20131210 when opening a .pen file on the portable environment, it doesn't
            // really make sense to query where the .dcf is, so we'll return the application
            // path (which is, in most cases, where the dictionary is located)
            symbol_file_path = interpreter.GetCurrentApplicationFilePath();
#endif
        }

        else if( symbol.IsA(SymbolType::Pre80Dictionary) )
        {
#ifdef WIN_DESKTOP
            symbol_file_path = assert_cast<const DICT&>(symbol).GetDataDict()->GetFilePath();
#else
            // 20131210 when opening a .pen file on the portable environment, it doesn't
            // really make sense to query where the .dcf is, so we'll return the application
            // path (which is, in most cases, where the dictionary is located)
            symbol_file_path = interpreter.GetCurrentApplicationFilePath();
#endif
        }

        // a file
        else
        {
            ASSERT(symbol.IsA(SymbolType::File));
            symbol_file_path = assert_cast<const LogicFile&>(symbol).GetFilePath();
        }

        return Get_DVE_RV<AllowSpecialDirectories>(PortableFunctions::PathGetDirectory(symbol_file_path));
    }


    // --------------------------------------------------------------------------
    // path types
    // --------------------------------------------------------------------------
    if( directory_variant_node.type == Nodes::DirectoryVariant::Type::Path )
    {
        switch( static_cast<Nodes::DirectoryVariant::PathType>(directory_variant_node.code_or_expression) )
        {
            case Nodes::DirectoryVariant::PathType::Temp:
                return Get_DVE_RV<AllowSpecialDirectories>(SharableString::FromStaticStringPointer(&GetTempDirectory()));

            case Nodes::DirectoryVariant::PathType::Application:
                return Get_DVE_RV<AllowSpecialDirectories>(PortableFunctions::PathGetDirectory(interpreter.GetCurrentApplicationFilePath()));

            case Nodes::DirectoryVariant::PathType::InputFile:
            {
                const ConnectionString& connection_string = interpreter.GetEngineData().dictionaries_pre80.front()->GetDicX()->GetDataRepository().GetConnectionString();
                return Get_DVE_RV<AllowSpecialDirectories>(connection_string.HasFilePath()
                    ? SharableString(PortableFunctions::PathGetDirectory(connection_string.GetFilePath()))
                    : SharableString());
            }

            case Nodes::DirectoryVariant::PathType::CSPro:
                return Get_DVE_RV<AllowSpecialDirectories>(SharableString::FromStaticStringPointer(&CSProExecutables::GetApplicationDirectory()));

#ifdef WIN_DESKTOP
            case Nodes::DirectoryVariant::PathType::Desktop:
                return Get_DVE_RV<AllowSpecialDirectories>(GetWindowsSpecialFolder(WindowsSpecialFolder::Desktop));

            case Nodes::DirectoryVariant::PathType::Windows:
                return Get_DVE_RV<AllowSpecialDirectories>(GetWindowsSpecialFolder(WindowsSpecialFolder::Windows));

            case Nodes::DirectoryVariant::PathType::Documents:
                return Get_DVE_RV<AllowSpecialDirectories>(GetWindowsSpecialFolder(WindowsSpecialFolder::Documents));

            case Nodes::DirectoryVariant::PathType::ProgramFiles32:
                return Get_DVE_RV<AllowSpecialDirectories>(GetWindowsSpecialFolder(WindowsSpecialFolder::ProgramFiles32));

            case Nodes::DirectoryVariant::PathType::ProgramFiles64:
                return Get_DVE_RV<AllowSpecialDirectories>(GetWindowsSpecialFolder(WindowsSpecialFolder::ProgramFiles64));
#else
            case Nodes::DirectoryVariant::PathType::CSEntry:
                return Get_DVE_RV<AllowSpecialDirectories>(SharableString::FromStaticStringPointer(&PlatformInterface::GetInstance()->GetCSEntryDirectory()));

            case Nodes::DirectoryVariant::PathType::CSEntryExternal:
                return Get_DVE_RV<AllowSpecialDirectories>(SharableString::FromStaticStringPointer(&PlatformInterface::GetInstance()->GetExternalMemoryCardDirectory()));
#endif
            case Nodes::DirectoryVariant::PathType::Html:
                return Get_DVE_RV<AllowSpecialDirectories>(SharableString::FromStaticStringPointer(&Html::GetDirectory()));

            case Nodes::DirectoryVariant::PathType::Downloads:
                return Get_DVE_RV<AllowSpecialDirectories>(SharableString::FromStaticStringPointer(&GetDownloadsDirectory()));

            default:
                return Get_DVE_RV<AllowSpecialDirectories>(SharableString());
        }
    }


    // --------------------------------------------------------------------------
    // media types
    // --------------------------------------------------------------------------
    if constexpr(AllowSpecialDirectories)
    {
        if( directory_variant_node.type == Nodes::DirectoryVariant::Type::Media )
        {
            return SpecialDirectoryLister::MediaStoreDirectory
            {
                static_cast<MediaStore::MediaType>(directory_variant_node.code_or_expression),
                std::string()
            };
        }
    }


    return Get_DVE_RV<AllowSpecialDirectories>(SharableString());
}


SharableString PathRT::PathFilterEvaluator(LogicInterpreter& interpreter, const int filter_type_or_expression)
{
    if( filter_type_or_expression == -1 )
    {
        // no filter
    }

    else if( filter_type_or_expression >= 0 )
    {
        std::string filter = SpecialDirectoryLister::EvaluateFilter(
            interpreter.Evaluate<SharableString>(filter_type_or_expression).GetString()
        );

        if( !filter.empty() )
            return filter;
    }

    else
    {
        const Nodes::Path::FilterType filter_type = static_cast<Nodes::Path::FilterType>(filter_type_or_expression);
        ASSERT(filter_type == Nodes::Path::FilterType::Audio ||
               filter_type == Nodes::Path::FilterType::Geometry ||
               filter_type == Nodes::Path::FilterType::Image ||
               filter_type == Nodes::Path::FilterType::Video);

        return SpecialDirectoryLister::EvaluateFilter(filter_type);
    }

    return SharableString();
}


Engine::Value LogicInterpreter::ex_pathname(const int program_index)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);

    SharableString full_path = PathRT::DirectoryVariantEvaluator<false>(*this, va_node.arguments[0]);

    if( full_path->empty() )
        return SharableString(PathRT::InvalidPathText_sv);

    return PortableFunctions::PathEnsureTrailingSlash(full_path.Release());
}


Engine::Value LogicInterpreter::ex_Path_concat(const int program_index)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);
    const int number_arguments = va_node.arguments[0];
    SharableString full_path = PathRT::DirectoryVariantEvaluator<false>(*this, va_node.arguments[1]);

    // prevent invalid calls on the wrong platform, like Path.concat(CSEntry, "a.txt") on Windows
    if( !full_path->empty() )
    {
        for( int i = 2; i <= number_arguments; ++i )
        {
            const SharableString this_entity = Evaluate<SharableString>(va_node.arguments[i]);

            if( full_path->empty() )
            {
                full_path = this_entity;
                MakeAbsolutePath(full_path.MakeModifiable());
            }

            else
            {
                full_path = MakeFullPath(*full_path, *this_entity);
            }
        }
    }

    if( full_path->empty() )
        return SharableString(PathRT::InvalidPathText_sv);

    return full_path;
}


Engine::Value LogicInterpreter::ex_Path_getDirectoryName(const int program_index)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);
    const std::string path = EvaluatePath(va_node.arguments[0]);
    return PortableFunctions::PathGetDirectory(path);
}


Engine::Value LogicInterpreter::ex_Path_getExtension(const int program_index)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);
    const SharableString path = Evaluate<SharableString>(va_node.arguments[0]);
    return PortableFunctions::PathGetFileExtension(*path, true);
}


Engine::Value LogicInterpreter::ex_Path_getFileName(const int program_index)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);
    const SharableString path = Evaluate<SharableString>(va_node.arguments[0]);
    return PortableFunctions::PathGetFilename(*path);
}


Engine::Value LogicInterpreter::ex_Path_getFileNameWithoutExtension(const int program_index)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);
    const SharableString path = Evaluate<SharableString>(va_node.arguments[0]);
    return Path::GetFilenameWithoutExtension(*path);
}


Engine::Value LogicInterpreter::ex_Path_getRelativePath(const int program_index)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);
    SharableString relative_to = PathRT::DirectoryVariantEvaluator<false>(*this, va_node.arguments[0]);

    std::string relative_path = EvaluatePath(va_node.arguments[1]);

    if( relative_to->empty() )
        return relative_path;

    // GetRelativePathForDisplay expects a file, not a directory, so create a fake filename
    return GetRelativePathForDisplay(
        Path::Combine(relative_to.Release(), "g"),
        relative_path
    );
}


Engine::Value LogicInterpreter::ex_Path_selectFile(const int program_index)
{
    const auto& path_select_file_node = GetNode<Nodes::PathSelectFile>(program_index);

    SelectFileDlg select_file_dlg;

    // evaluate the title
    if( path_select_file_node.title_expression != -1 )
    {
        select_file_dlg.SetTitle(
            Evaluate<SharableString>(path_select_file_node.title_expression)
        );
    }

    // evaluate whether to show directories
    select_file_dlg.SetShowDirectories(
        EvaluateOptionalConditional(path_select_file_node.show_directories_expression, true)
    );

    try
    {
        // evaluate the filter
        select_file_dlg.SetFilter(
            PathRT::PathFilterEvaluator(*this, path_select_file_node.filter_type_or_expression)
        );
    }

    catch( const std::exception& exception )
    {
        IssueMessage(MessageType::Error, MGF::Path_invalid_directory_or_filter_100373, exception.what());
        return Engine::Value::Invalid<SharableString>();
    }


    // evaluate the start and root directories
    auto evaluate_directory = [&](const int directory_variant_index) -> std::optional<SpecialDirectoryLister::SpecialDirectory>
    {
        if( directory_variant_index == -1 )
            return std::nullopt;

        SpecialDirectoryLister::SpecialDirectory special_directory =
            PathRT::DirectoryVariantEvaluator<true>(*this, directory_variant_index);

        if( std::holds_alternative<std::string>(special_directory) )
        {
            if( PortableFunctions::FileIsDirectory(std::get<std::string>(special_directory)) )
                return PortableFunctions::PathRemoveTrailingSlash(std::get<std::string>(special_directory));

            throw FileIO::Exception::DirectoryNotFound(std::get<std::string>(special_directory));
        }

        return special_directory;
    };

    try
    {
        std::optional<SpecialDirectoryLister::SpecialDirectory> start_directory = evaluate_directory(path_select_file_node.start_directory_variant_index);
        std::optional<SpecialDirectoryLister::SpecialDirectory> root_directory = evaluate_directory(path_select_file_node.root_directory_variant_index);

        if( !start_directory.has_value() )
        {
            // if a root directory is specified but not a start directory, use the root as the start
            if( root_directory.has_value() )
            {
                start_directory = root_directory;
            }

            // otherwise use the application directory
            else
            {
                start_directory = PortableFunctions::PathGetDirectory(GetCurrentApplicationFilePath());
            }
        }

        select_file_dlg.SetStartDirectory(*start_directory);

        if( root_directory.has_value() )
        {
            // make sure the start directory is within the root directory
            try
            {
                SpecialDirectoryLister::ValidateStartAndRootSpecialDirectories(*start_directory, *root_directory);
            }

            catch(...)
            {
                // match pre-8.0 behavior where the root directory will be set to the start directory (with a warning) when possible
                if( start_directory->index() == root_directory->index() )
                {
                    IssueMessage(MessageType::Warning, MGF::Path_start_root_directory_error_100374,
                                 SpecialDirectoryLister::GetSpecialDirectoryPath(*start_directory).c_str(),
                                 SpecialDirectoryLister::GetSpecialDirectoryPath(*root_directory).c_str());

                    root_directory = start_directory;
                }

                else
                {
                    throw;
                }
            }

            select_file_dlg.SetRootDirectory(std::move(*root_directory));
        }
    }

    catch( const std::exception& exception )
    {
        IssueMessage(MessageType::Error, MGF::Path_file_selection_error_100372, exception.what());
        return Engine::Value::Invalid<SharableString>();
    }


    // show the dialog
    select_file_dlg.DoModalOnUIThread();

    return select_file_dlg.GetSelectedPath();
}



// --------------------------------------------------------------------------
// directory functions
// --------------------------------------------------------------------------

Engine::Value LogicInterpreter::ex_direxist(const int program_index)
{
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);
    const std::string directory = EvaluatePath(fnn_node.fn_expr[0]);

    return Engine::Value::Bool(
        PortableFunctions::FileIsDirectory(directory)
    );
}


Engine::Value LogicInterpreter::ex_dircreate(const int program_index)
{
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);
    const std::string directory = EvaluatePath(fnn_node.fn_expr[0]);

    return Engine::Value::Bool(
        PortableFunctions::PathMakeDirectories(directory)
    );
}


Engine::Value LogicInterpreter::ex_dirdelete(const int program_index)
{
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);

    std::string directory = PortableFunctions::PathRemoveTrailingSlash(EvaluatePath(fnn_node.fn_expr[0]));
    std::string parent_directory = PortableFunctions::PathGetDirectory(directory);
    std::string directory_name = PortableFunctions::PathGetFilename(directory);

    const std::vector<std::string> directories =
        DirectoryLister()
        .SetIncludeFiles(false)
        .SetIncludeDirectories(true)
        .SetNameFilter(directory_name)
        .GetPaths(parent_directory);

    // indicate that the directory was not valid if no directories matched when not using wildcards
    if( directories.empty() && !Path::Path::HasWildcardCharacters(directory_name) )
        return Engine::Value::Invalid<double>();

    size_t directories_deleted = 0;

    for( const std::string& this_directory : directories )
    {
        if( PortableFunctions::DirectoryDelete(this_directory) )
            ++directories_deleted;
    }

    return ( directories_deleted == directories.size() )
        ? Engine::Value::Integer(directories_deleted)
        : Engine::Value::Invalid<double>();
}


Engine::Value LogicInterpreter::ex_dirlist(const int program_index)
{
    const auto& dirlist_node = GetNode<Nodes::DirList>(program_index);

    LogicList& logic_list = GetSymbolLogicList(dirlist_node.list_symbol_index);

    if( logic_list.IsReadOnly() )
    {
        IssueMessage(MessageType::Error, MGF::List_read_only_cannot_be_modified_965, logic_list.GetName().c_str());
        return Engine::Value::Invalid<double>();
    }

    logic_list.Reset();

    try
    {
        const SpecialDirectoryLister::SpecialDirectory special_directory =
            PathRT::DirectoryVariantEvaluator<true>(*this, dirlist_node.directory_variant_index);
        const SharableString filter = PathRT::PathFilterEvaluator(*this, dirlist_node.filter_type_or_expression);
        const bool recursive = EvaluateOptionalConditional(dirlist_node.recursive_expression, false);
        constexpr bool include_files = true;

        // process regular directories
        if( std::holds_alternative<std::string>(special_directory) )
        {
            if( !PortableFunctions::FileIsDirectory(std::get<std::string>(special_directory)) )
                return Engine::Value::Bool(false);

            constexpr bool include_directories = true;

            DirectoryLister directory_lister(recursive, include_files, include_directories);

            if( filter.IsSet() )
                directory_lister.SetNameFilter(*filter);

            logic_list.AddValues(directory_lister.GetPaths(std::get<std::string>(special_directory)));
        }

        // process media directories
        else if( std::holds_alternative<SpecialDirectoryLister::MediaStoreDirectory>(special_directory) )
        {
            // set recursive to true so that the media files are not returned sorted into their virtual directories
            constexpr bool recursive_override = true;
            constexpr bool include_directories = false;

            const std::unique_ptr<SpecialDirectoryLister> special_directory_lister =
                SpecialDirectoryLister::CreateSpecialDirectoryLister(
                    std::get<SpecialDirectoryLister::MediaStoreDirectory>(special_directory),
                    recursive_override,
                    include_files,
                    include_directories
                );
            ASSERT(special_directory_lister != nullptr);

            if( filter.IsSet() )
                special_directory_lister->SetNameFilter(*filter);

            logic_list.AddValues(special_directory_lister->GetSpecialPaths());
        }

        else
        {
            ASSERT(std::holds_alternative<SpecialDirectoryLister::AndroidRoot>(special_directory));
        }
    }

    catch( const std::exception& exception )
    {
        IssueMessage(MessageType::Error, MGF::Path_invalid_directory_or_filter_100373, exception.what());
        return Engine::Value::Bool(false);
    }

    return Engine::Value::Bool(true);
}



// --------------------------------------------------------------------------
// file functions
// --------------------------------------------------------------------------

namespace PathRT
{
    // Returns the evaluated path or file path of a LogicFile object.
    std::string EvaluatePathOrSymbolPath(LogicInterpreter& interpreter, int program_index);

    // Returns the evaluated paths or file paths of a LogicFile or LogicList object.
    // T can be std::string or ConnectionString.
    template<typename T>
    std::vector<T> EvaluatePathsOrSymbolPaths(LogicInterpreter& interpreter, int program_index,
                                              bool evaluate_paths_with_wildcard_support, bool* paths_use_wildcards);

    class EngineConcatenatorReporter;
}


std::string PathRT::EvaluatePathOrSymbolPath(LogicInterpreter& interpreter, const int program_index)
{
    if( program_index >= 0 )
    {
        return interpreter.EvaluatePath(program_index);
    }

    else
    {
        auto GetSymbolTable = [&]() -> const Logic::SymbolTable& { return interpreter.GetSymbolTable(); };

        const LogicFile& logic_file = GetSymbolLogicFile(-1 * program_index);
        ASSERT(logic_file.GetFilePath() == interpreter.GetAbsolutePath(logic_file.GetFilePath()));
        return logic_file.GetFilePath();
    }
}


template<typename T>
std::vector<T> PathRT::EvaluatePathsOrSymbolPaths(LogicInterpreter& interpreter, const int program_index,
                                                  const bool evaluate_paths_with_wildcard_support,
                                                  bool* const paths_use_wildcards)
{
    ASSERT(paths_use_wildcards == nullptr || ( evaluate_paths_with_wildcard_support && !*paths_use_wildcards ));

    std::vector<T> file_paths;

    if( program_index >= 0 )
    {
        T& file_path = file_paths.emplace_back(interpreter.Evaluate<std::string>(program_index));
        interpreter.MakeAbsolutePath(file_path);
    }

    else
    {
        auto GetSymbolTable = [&]() -> const Logic::SymbolTable& { return interpreter.GetSymbolTable(); };

        const Symbol& symbol = NPT_Ref(-1 * program_index);

        if( symbol.IsA(SymbolType::File) )
        {
            file_paths.emplace_back(assert_cast<const LogicFile&>(symbol).GetFilePath());

            if constexpr(std::is_same_v<T, std::string>)
            {
                ASSERT(file_paths.back() == interpreter.GetAbsolutePath(file_paths.back()));
            }
        }

        else
        {
            ASSERT(symbol.IsA(SymbolType::List));
            const LogicList& logic_list = assert_cast<const LogicList&>(symbol);
            const size_t list_count = logic_list.GetCount();

            for( size_t i = 1; i <= list_count; ++i )
            {
                T& file_path = file_paths.emplace_back(*logic_list.GetValue<SharableString>(i));
                interpreter.MakeAbsolutePath(file_path);
            }
        }
    }

    // remove any blank strings
    file_paths.erase(
        std::remove_if(file_paths.begin(), file_paths.end(),
            [](const T& file_path)
            {
                if constexpr(std::is_same_v<T, std::string>)
                {
                    return file_path.empty();
                }

                else
                {
                    return !file_path.IsDefined();
                }
            }),
        file_paths.end()
    );

    // potentially evaluate wildcards
    if( evaluate_paths_with_wildcard_support )
    {
        std::vector<T> provided_file_paths = std::exchange(file_paths, std::vector<T>());
        ASSERT(file_paths.empty());

        for( const T& provided_file_path : provided_file_paths )
        {
            if constexpr(std::is_same_v<T, std::string>)
            {
                DirectoryLister::AddFilePathsWithPossibleWildcard(file_paths, provided_file_path, true);

                if( paths_use_wildcards != nullptr && !*paths_use_wildcards )
                    *paths_use_wildcards = Path::HasWildcardCharacters(Path::GetFilename(provided_file_path));
            }

            else
            {
                PathHelpers::ExpandConnectionStringWildcards(file_paths, provided_file_path);
                ASSERT(paths_use_wildcards == nullptr);
            }
        }
    }

    return file_paths;
}


Engine::Value LogicInterpreter::ex_fileexist(const int program_index)
{
    const auto& file_node = GetNode<Nodes::File>(program_index);
    const std::string file_path = PathRT::EvaluatePathOrSymbolPath(*this, file_node.symbol_index_or_string_expression);

    return Engine::Value::Bool(
        file_path.empty()                      ? false :
        Path::HasWildcardCharacters(file_path) ? !DirectoryLister::GetFilePathsWithPossibleWildcard(file_path, false).empty() :
                                                 PortableFunctions::FileIsRegular(file_path)
    );
}


Engine::Value LogicInterpreter::ex_fileempty(const int program_index)
{
    const auto& file_node = GetNode<Nodes::File>(program_index);

    // if the file is open, use the file handle to do the check
    if( file_node.symbol_index_or_string_expression < 0 )
    {
        LogicFile& logic_file = GetSymbolLogicFile(-1 * file_node.symbol_index_or_string_expression);

        if( logic_file.IsOpen() )
        {
            CFile& file = logic_file.GetFile();

            if( file.GetLength() == 0 )
            {
                return Engine::Value::Bool(true);
            }

            else if( file.GetLength() == TextEncoding::Utf8Bom_sv.length() )
            {
                const ULONGLONG position = file.GetPosition();
                file.SeekToBegin();

                auto content = std::make_unique_for_overwrite<char[]>(TextEncoding::Utf8Bom_sv.length());
                const size_t bytes_read = file.Read(content.get(), TextEncoding::Utf8Bom_sv.length());

                file.Seek(position, CFile::begin);

                if( bytes_read == TextEncoding::Utf8Bom_sv.length() &&
                    memcmp(content.get(), TextEncoding::Utf8Bom_sv.data(), TextEncoding::Utf8Bom_sv.length()) == 0 )
                {
                    return Engine::Value::Bool(true);
                }
            }

            return Engine::Value::Bool(false);
        }
    }

    // process a provided file path (or the file path of an unopened file handle)
    const std::string file_path = PathRT::EvaluatePathOrSymbolPath(*this, file_node.symbol_index_or_string_expression);
    const size_t file_size = static_cast<size_t>(PortableFunctions::FileSize(file_path));

    if( file_size == 0 )
    {
        return Engine::Value::Bool(true);
    }

    else if( file_size == TextEncoding::Utf8Bom_sv.length() )
    {
        try
        {
            const BinaryBlock content = FileIO::ReadBinary(file_path);

            if( content.size() == TextEncoding::Utf8Bom_sv.length() &&
                memcmp(content.data(), TextEncoding::Utf8Bom_sv.data(), TextEncoding::Utf8Bom_sv.length()) == 0 )
            {
                return Engine::Value::Bool(true);
            }
        }

        catch( const FileIO::Exception& )
        {
            // if there are errors reading the content, treat the file as not empty
        }
    }

    return ( file_size != -1 )
        ? Engine::Value::Bool(false)
        : Engine::Value::Invalid<double>();
}


Engine::Value LogicInterpreter::ex_filesize(const int program_index)
{
    const auto& file_node = GetNode<Nodes::File>(program_index);
    const std::string file_path = PathRT::EvaluatePathOrSymbolPath(*this, file_node.symbol_index_or_string_expression);

    if( !file_path.empty() )
    {
        const int64_t file_size = PortableFunctions::FileSize(file_path);

        if( file_size != -1 )
            return Engine::Value::Integer(file_size);
    }

    return Engine::Value::Invalid<double>();
}


Engine::Value LogicInterpreter::ex_filetime(const int program_index)
{
    const auto& file_node = GetNode<Nodes::File>(program_index);
    const std::string file_path = PathRT::EvaluatePathOrSymbolPath(*this, file_node.symbol_index_or_string_expression);

    if( !file_path.empty() )
    {
        const int64_t file_modified_time = PortableFunctions::FileModifiedTime(file_path);

        if( file_modified_time != 0 )
            return Engine::Value::Integer(file_modified_time);
    }

    return Engine::Value::Invalid<double>();
}


Engine::Value LogicInterpreter::ex_filename(const int program_index)
{
    const auto& fn8_node = GetNode<FN8_NODE>(program_index);

    // the paradata log
    if( fn8_node.symbol_index == -2 )
        return Paradata::Logger::GetFilePath();

    // symbols
    Symbol* const symbol = GetFromSymbolOrEngineItem(
        fn8_node.symbol_index,
        m_engineData->MeetsCompiledLogicVersion(Serializer::Iteration_8_0_000_1) ? fn8_node.extra_parameter : -1
    );

    if( symbol == nullptr )
        return Engine::Value::Invalid<SharableString>();

    switch( symbol->GetType() )
    {
        // dictionary
        case SymbolType::Dictionary:
        {
            const EngineDictionary& engine_dictionary = assert_cast<const EngineDictionary&>(*symbol);
            const ConnectionString& connection_string = engine_dictionary.GetEngineDataRepository().GetDataRepository().GetConnectionString();
            return connection_string.HasFilePath() ? connection_string.GetFilePath() :
                                                     Engine::Value::Undefined<SharableString>();
        }

        case SymbolType::Pre80Dictionary:
        {
            const DICT* const pDicT = assert_cast<const DICT*>(symbol);
            const DICX* const pDicX = pDicT->GetDicX();
            const ConnectionString& connection_string = pDicX->GetDataRepository().GetConnectionString();
            return connection_string.HasFilePath() ? connection_string.GetFilePath() :
                                                     Engine::Value::Undefined<SharableString>();
        }

        // File
        case SymbolType::File:
        {
            const LogicFile& logic_file = assert_cast<const LogicFile&>(*symbol);
            return logic_file.GetFilePath();
        }

        // Pff
        case SymbolType::Pff:
        {
            LogicPff& logic_pff = assert_cast<LogicPff&>(*symbol);
            return logic_pff.GetRunnableFilePath();
        }

        // Report
        case SymbolType::Report:
        {
            const Report& report = assert_cast<const Report&>(*symbol);
            return report.GetFilePath();
        }
    }

    // Audio, Document, Geometry, Image, Video
    if( BinarySymbol::IsBinarySymbol(*symbol) )
        return assert_cast<const BinarySymbol&>(*symbol).GetPath();

    return Engine::Value::Invalid<SharableString>();
}


Engine::Value LogicInterpreter::ex_filecreate(const int program_index)
{
    const auto& file_node = GetNode<Nodes::File>(program_index);
    const std::string file_path = PathRT::EvaluatePathOrSymbolPath(*this, file_node.symbol_index_or_string_expression);

    if( !file_path.empty() )
    {
        try
        {
            FileIO::WriteText(file_path, std::string_view(), true);
            return Engine::Value::Bool(true);
        }
        catch( const FileIO::Exception& ) { }
    }

    return Engine::Value::Bool(false);
}


Engine::Value LogicInterpreter::ex_filedelete(const int program_index)
{
    const auto& file_node = GetNode<Nodes::File>(program_index);
    const std::vector<std::string> file_paths = PathRT::EvaluatePathsOrSymbolPaths<std::string>(
        *this,
        file_node.symbol_index_or_string_expression,
        true, // evaluate_paths_with_wildcard_support
        nullptr // paths_use_wildcards
    );
    size_t files_deleted = 0;

    for( const std::string& file_path : file_paths )
    {
        if( PortableFunctions::FileDelete(file_path) )
            ++files_deleted;
    }

    return ( files_deleted == file_paths.size() )
        ? Engine::Value::Integer(files_deleted)
        : Engine::Value::Invalid<double>();
}


Engine::Value LogicInterpreter::ex_filecopy_filerename(const int program_index)
{
    const auto& file_node = GetNode<Nodes::File>(program_index);
    const Nodes::List& elements_list = GetListNode(file_node.elements_list_node);
    const std::string output_path = PathRT::EvaluatePathOrSymbolPath(*this, elements_list.elements[0]);

    if( output_path.empty() )
        return Engine::Value::Invalid<double>();

    if( Path::HasWildcardCharacters(Path::GetFilename(output_path)) )
    {
        IssueMessage(MessageType::Error, MGF::path_wildcard_not_supported_33056, output_path.c_str());
        return Engine::Value::Invalid<double>();
    }

    const bool output_is_folder = PortableFunctions::FileIsDirectory(output_path);

    bool paths_use_wildcards = false;
    const std::vector<std::string> input_file_paths = PathRT::EvaluatePathsOrSymbolPaths<std::string>(
        *this,
        file_node.symbol_index_or_string_expression,
        true, // evaluate_paths_with_wildcard_support
        &paths_use_wildcards
    );

    // if a wildcard is used in the input, then the output must be a folder that exists
    if( paths_use_wildcards && !output_is_folder )
    {
        IssueMessage(MessageType::Error, MGF::path_target_directory_does_not_exist_33057, output_path.c_str());
        return Engine::Value::Invalid<double>();
    }

    const bool copying = ( file_node.function_code == FunctionCode::FNFILE_COPY_CODE );
    ASSERT(copying || ( file_node.function_code == FunctionCode::FNFILE_RENAME_CODE ));

    size_t files_processed = 0;

    for( const std::string& input_file_path : input_file_paths )
    {
        const std::string output_file_path = !output_is_folder
            ? output_path
            : Path::Combine(output_path, PortableFunctions::PathGetFilename(input_file_path));

        try
        {
            if( copying )
            {
                PortableFileSystem::FileCopy(input_file_path, output_file_path, FileOverwriteFlag::Different);
            }

            else
            {
                PortableFunctions::FileRenameWithExceptions(input_file_path, output_file_path);
            }

            ++files_processed;
        }
        catch(...) { }
    }

    return ( files_processed == input_file_paths.size() )
        ? Engine::Value::Integer(files_processed)
        : Engine::Value::Invalid<double>();
}


class PathRT::EngineConcatenatorReporter : public ConcatenatorReporter
{
public:
    EngineConcatenatorReporter(LogicInterpreter& interpreter, const CDataDict* const dictionary)
        :   ConcatenatorReporter(( dictionary != nullptr ) ? dictionary->CreateProcessSummary() : std::make_unique<ProcessSummary>()),
            m_interpreter(interpreter)
    {
    }

    bool IsCanceled() const override
    {
        return m_interpreter.Get_m_bStopExec_INTERPRETER_DLL_TODO();
    }

    // progress reporting does not exist when invoked from fileconcat
    void SetSource(const std::string& /*source_text*/) override { }
    void SetKey(const std::string& /*key*/) override { }

    void ErrorFileOpenFailed(const std::string& file_path) override
    {
        m_interpreter.IssueMessage(MessageType::Error, MGF::cannot_open_file_2001, file_path.c_str());
    }

    void ErrorDataSourceOpenFailed(const ConnectionString& connection_string, const std::string& error_message) override
    {
        m_interpreter.IssueMessage(MessageType::Error, MGF::cannot_open_file_2001, SO::CreateParentheticalExpression(connection_string.ToDisplayString(), error_message).c_str());
    }

    void ErrorInvalidEncoding(const std::string& file_path) override
    {
        m_interpreter.IssueMessage(MessageType::Error, MGF::concat_text_file_encoding_not_supported_14012, file_path.c_str());
    }

    void ErrorDuplicateCase(const std::string& /*key*/, const ConnectionString& /*connection_string*/, const ConnectionString& /*previous_connection_string*/) override
    {
        // ignore duplicate cases as it would be very annoying to show a message for each one
    }

    void ErrorOther(const ConnectionString& connection_string, const std::string& error_message) override
    {
        m_interpreter.IssueMessage(MessageType::Error, MGF::concat_data_source_error_14013, connection_string.ToDisplayString().c_str(), error_message.c_str());
    }

private:
    LogicInterpreter& m_interpreter;
};


Engine::Value LogicInterpreter::ex_fileconcat(const int program_index)
{
    const auto& file_node = GetNode<Nodes::File>(program_index);
    const Nodes::List& elements_list = GetListNode(file_node.elements_list_node);

    // if the first argument is a dictionary then this is case concat using the dictionary;
    // otherwise it is just a filename and we use text concat
    std::shared_ptr<const CDataDict> dictionary;
    std::unique_ptr<EngineCaseConstructionReporter> case_construction_reporter;

    int output_file_expression;
    int input_file_start_position;

    // case concat
    if( file_node.symbol_index_or_string_expression < 0 )
    {
        const Symbol& symbol = NPT_Ref(-1 * file_node.symbol_index_or_string_expression);

        if( symbol.IsA(SymbolType::Dictionary) )
        {
            dictionary = assert_cast<const EngineDictionary&>(symbol).GetSharedDictionary();
        }

        else
        {
            dictionary = assert_cast<const DICT&>(symbol).GetSharedDictionary();
        }

        case_construction_reporter = std::make_unique<EngineCaseConstructionReporter>(
            m_engineData->engine_accessor->ea_GetSharedSystemMessageIssuer(),
            nullptr // process_summary
        );

        output_file_expression = elements_list.elements[0];
        input_file_start_position = 1;
    }

    // text concat
    else
    {
        output_file_expression = file_node.symbol_index_or_string_expression;
        input_file_start_position = 0;
    }

    const ConnectionString output_connection_string = EvaluateConnectionString(output_file_expression);

    std::vector<ConnectionString> input_connection_strings;

    for( int i = input_file_start_position; i < elements_list.number_elements; ++i )
    {
        std::vector<ConnectionString> these_input_connection_strings = PathRT::EvaluatePathsOrSymbolPaths<ConnectionString>(
            *this,
            elements_list.elements[i],
            true, // evaluate_paths_with_wildcard_support
            nullptr // paths_use_wildcards
        );

        input_connection_strings.insert(
            input_connection_strings.end(),
            std::make_move_iterator(these_input_connection_strings.begin()),
            std::make_move_iterator(these_input_connection_strings.end())
        );
    }

    if( input_connection_strings.empty() )
        return Engine::Value::Bool(false);

    try
    {
        Concatenator concatenator;
        PathRT::EngineConcatenatorReporter engine_concatenator_reporter(*this, dictionary.get());

        concatenator.Run(
            engine_concatenator_reporter,
            input_connection_strings,
            output_connection_string,
            dictionary,
            std::move(case_construction_reporter)
        );

        return Engine::Value::Bool(true);
    }

    catch( const CSProException& exception )
    {
        IssueMessage(MessageType::Error, MGF::concat_error_14011, exception.what());
        return Engine::Value::Bool(false);
    }
}
