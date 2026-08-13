#include "stdafx.h"
#include "IncludesRT.h"
#include "EngineDictionary.h"
#include "File.h"
#include "List.h"
#include "Nodes/Path.h"
#include <engine/DicT.h>
#include <engine/DicX.h>
#include <zUtilO/CSProExecutables.h>
#include <zUtilO/SpecialDirectoryLister.h>
#include <zUtilF/SelectFileDlg.h>


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
