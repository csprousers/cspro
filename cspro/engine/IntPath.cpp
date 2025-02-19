#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include "Engine.h"
#include <zEngineO/EngineDictionary.h>
#include <zEngineO/File.h>
#include <zEngineO/List.h>
#include <zEngineO/Messages/EngineMessages.h>
#include <zEngineO/Nodes/Path.h>
#include <zPlatformO/PlatformInterface.h>
#include <zToolsO/FileIO.h>
#include <zUtilO/CSProExecutables.h>
#include <zUtilO/SpecialDirectoryLister.h>
#include <zUtilF/SelectFileDlg.h>
#include <ZBRIDGEO/npff.h>


namespace
{
    constexpr std::string_view InvalidPathText_sv = "<invalid path>";


    template<typename RV, typename T>
    RV UTF8_TODO_PREPROCESS(T&& value)
    {
        if constexpr(std::is_same_v<RV, SpecialDirectoryLister::SpecialDirectory> &&
                     std::is_same_v<std::remove_cvref_t<T>, SharableString>)
        {
            return *value;
        }

        else if constexpr(std::is_same_v<RV, SpecialDirectoryLister::SpecialDirectory> &&
                          std::is_same_v<std::remove_cvref_t<T>, std::wstring>)
        {
            return UTF8_TODO::GetUtf8(value);
        }

        else
        {
            return std::forward<T>(value);
        }
    }


    template<bool AllowSpecialDirectories, typename CN,
             typename RV = typename std::conditional<AllowSpecialDirectories, SpecialDirectoryLister::SpecialDirectory, SharableString>::type>
    RV DirectoryVariantEvaluator(CIntDriver& interpreter, const CN& directory_variant_index_or_node)
    {
        CEngineArea* m_pEngineArea = interpreter.m_pEngineArea;
        CEngineDriver* m_pEngineDriver = interpreter.m_pEngineDriver;

        auto GetSymbolTable = [&]() -> const Logic::SymbolTable& { return interpreter.GetSymbolTable(); };

        auto get_directory_variant_node = [&]() -> const Nodes::DirectoryVariant&
        {
            if constexpr(std::is_same_v<CN, int>)
            {
                return interpreter.GetNode<Nodes::DirectoryVariant>(directory_variant_index_or_node);
            }

            else
            {
                return directory_variant_index_or_node;
            }
        };

        const Nodes::DirectoryVariant& directory_variant_node = get_directory_variant_node();

        // --------------------------------------------------------------------------
        // string expressions
        // --------------------------------------------------------------------------
        if( directory_variant_node.type == Nodes::DirectoryVariant::Type::String )
        {
            std::string filename_or_path = interpreter.EvaluateString(directory_variant_node.code_or_expression);

            if constexpr(AllowSpecialDirectories)
            {
                if( SpecialDirectoryLister::IsSpecialDirectory(filename_or_path) )
                    return SpecialDirectoryLister::EvaluateSpecialDirectory(filename_or_path);
            }

            interpreter.MakeAbsolutePath(filename_or_path);

            return UTF8_TODO_PREPROCESS<RV>(std::move(filename_or_path));
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
                // path (which is, in some senses, where the dictionary is located)
                symbol_file_path = UTF8_TODO::GetUtf8(m_pEngineDriver->m_pPifFile->GetAppFName());
#endif
            }

            else if( symbol.IsA(SymbolType::Pre80Dictionary) )
            {
#ifdef WIN_DESKTOP
                symbol_file_path = assert_cast<const DICT&>(symbol).GetDataDict()->GetFilePath();
#else
                // 20131210 when opening a .pen file on the portable environment, it doesn't
                // really make sense to query where the .dcf is, so we'll return the application
                // path (which is, in some senses, where the dictionary is located)
                symbol_file_path = UTF8_TODO::GetUtf8(m_pEngineDriver->m_pPifFile->GetAppFName());
#endif
            }

            // a file
            else
            {
                ASSERT(symbol.IsA(SymbolType::File));
                symbol_file_path = assert_cast<const LogicFile&>(symbol).GetFilePath();
            }

            return UTF8_TODO_PREPROCESS<RV>(UTF8_TODO::GetUtf8(GetFilePath(UTF8_TODO::GetCString(symbol_file_path))));
        }


        // --------------------------------------------------------------------------
        // path types
        // --------------------------------------------------------------------------
        if( directory_variant_node.type == Nodes::DirectoryVariant::Type::Path )
        {
            switch( static_cast<Nodes::DirectoryVariant::PathType>(directory_variant_node.code_or_expression) )
            {
                case Nodes::DirectoryVariant::PathType::Temp:
                    return UTF8_TODO_PREPROCESS<RV>(SharableString::FromStaticStringPointer(&GetTempDirectory()));

                case Nodes::DirectoryVariant::PathType::Application:
                    return UTF8_TODO_PREPROCESS<RV>(UTF8_TODO::GetUtf8(GetFilePath(m_pEngineDriver->m_pPifFile->GetAppFName())));

                case Nodes::DirectoryVariant::PathType::InputFile:
                {
                    const ConnectionString& connection_string = m_pEngineArea->GetEngineData().dictionaries_pre80.front()->GetDicX()->GetDataRepository().GetConnectionString();
                    return UTF8_TODO_PREPROCESS<RV>(connection_string.HasFilePath() ? SharableString(PortableFunctions::PathGetDirectory(connection_string.GetFilePath())) :
                                                                                      SharableString());
                }

                case Nodes::DirectoryVariant::PathType::CSPro:
                    return UTF8_TODO_PREPROCESS<RV>(SharableString::FromStaticStringPointer(&CSProExecutables::GetApplicationDirectory()));

#ifdef WIN_DESKTOP
                case Nodes::DirectoryVariant::PathType::Desktop:
                    return UTF8_TODO_PREPROCESS<RV>(GetWindowsSpecialFolder(WindowsSpecialFolder::Desktop));

                case Nodes::DirectoryVariant::PathType::Windows:
                    return UTF8_TODO_PREPROCESS<RV>(GetWindowsSpecialFolder(WindowsSpecialFolder::Windows));

                case Nodes::DirectoryVariant::PathType::Documents:
                    return UTF8_TODO_PREPROCESS<RV>(GetWindowsSpecialFolder(WindowsSpecialFolder::Documents));

                case Nodes::DirectoryVariant::PathType::ProgramFiles32:
                    return UTF8_TODO_PREPROCESS<RV>(GetWindowsSpecialFolder(WindowsSpecialFolder::ProgramFiles32));

                case Nodes::DirectoryVariant::PathType::ProgramFiles64:
                    return UTF8_TODO_PREPROCESS<RV>(GetWindowsSpecialFolder(WindowsSpecialFolder::ProgramFiles64));
#else
                case Nodes::DirectoryVariant::PathType::CSEntry:
                    return UTF8_TODO_PREPROCESS<RV>(SharableString::FromStaticStringPointer(&PlatformInterface::GetInstance()->GetCSEntryDirectory()));

                case Nodes::DirectoryVariant::PathType::CSEntryExternal:
                    return UTF8_TODO_PREPROCESS<RV>(SharableString::FromStaticStringPointer(&PlatformInterface::GetInstance()->GetExternalMemoryCardDirectory()));
#endif
                case Nodes::DirectoryVariant::PathType::Html:
                    return UTF8_TODO_PREPROCESS<RV>(SharableString::FromStaticStringPointer(&Html::GetDirectory()));

                case Nodes::DirectoryVariant::PathType::Downloads:
                    return UTF8_TODO_PREPROCESS<RV>(SharableString::FromStaticStringPointer(&GetDownloadsDirectory()));

                default:
                    return UTF8_TODO_PREPROCESS<RV>(SharableString());
            }
        }


        // --------------------------------------------------------------------------
        // media types
        // --------------------------------------------------------------------------
        if constexpr(AllowSpecialDirectories)
        {
            if( directory_variant_node.type == Nodes::DirectoryVariant::Type::Media )
                return SpecialDirectoryLister::MediaStoreDirectory { static_cast<MediaStore::MediaType>(directory_variant_node.code_or_expression), std::string() };
        }


        return UTF8_TODO_PREPROCESS<RV>(SharableString());
    }


    SharableString PathFilterEvaluator(CIntDriver& interpreter, const int filter_type_or_expression)
    {
        if( filter_type_or_expression == -1 )
        {
            // no filter
        }

        else if( filter_type_or_expression >= 0 )
        {
            std::string filter = SpecialDirectoryLister::EvaluateFilter(*interpreter.EvaluateSharableString(filter_type_or_expression));

            if( !filter.empty() )
                return filter;
        }

        else
        {
            const Nodes::Path::FilterType filter_type = static_cast<Nodes::Path::FilterType>(filter_type_or_expression);
            ASSERT(filter_type == Nodes::Path::FilterType::Audio ||
                   filter_type == Nodes::Path::FilterType::Geometry ||
                   filter_type == Nodes::Path::FilterType::Image);

            return SpecialDirectoryLister::EvaluateFilter(filter_type);
        }

        return SharableString();
    }
}


double CIntDriver::ex_pathname(const int program_index)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);

    SharableString full_path = DirectoryVariantEvaluator<false>(*this, va_node.arguments[0]);

    if( !full_path->empty() )
        return AssignString(PortableFunctions::PathEnsureTrailingSlash(full_path.Release()));

    return AssignString(InvalidPathText_sv);
}


double CIntDriver::ex_Path_concat(const int program_index)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);
    const int& number_arguments = va_node.arguments[0];
    SharableString full_path = DirectoryVariantEvaluator<false>(*this, va_node.arguments[1]);

    // prevent invalid calls on the wrong platform, like Path.concat(CSEntry, "a.txt") on Windows
    if( !full_path->empty() )
    {
        for( int i = 2; i <= number_arguments; ++i )
        {
            const SharableString this_entity = EvaluateSharableString(va_node.arguments[i]);

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
        full_path = InvalidPathText_sv;

    return AssignString(std::move(full_path));
}


double CIntDriver::ex_Path_getDirectoryName(const int program_index)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);
    const std::string path = EvaluatePath(va_node.arguments[0]);
    return AssignString(PortableFunctions::PathGetDirectory(path));
}


double CIntDriver::ex_Path_getExtension(const int program_index)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);
    const SharableString path = EvaluateSharableString(va_node.arguments[0]);
    return AssignString(PortableFunctions::PathGetFileExtension(*path, true));
}


double CIntDriver::ex_Path_getFileName(const int program_index)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);
    const SharableString path = EvaluateSharableString(va_node.arguments[0]);
    return AssignString(PortableFunctions::PathGetFilename(*path));
}


double CIntDriver::ex_Path_getFileNameWithoutExtension(const int program_index)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);
    const SharableString path = EvaluateSharableString(va_node.arguments[0]);
    return AssignString(Path::GetFilenameWithoutExtension(*path));
}


double CIntDriver::ex_Path_getRelativePath(const int program_index)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);
    SharableString relative_to = DirectoryVariantEvaluator<false>(*this, va_node.arguments[0]);

    std::string relative_path = EvaluatePath(va_node.arguments[1]);

    if( !relative_to->empty() )
    {
        // GetRelativePathForDisplay expects a file, not a directory, so create a fake filename
        relative_path = GetRelativePathForDisplay(Path::Combine(relative_to.Release(), "g"),
                                                  relative_path);
    }

    return AssignString(std::move(relative_path));
}


double CIntDriver::ex_Path_selectFile(const int program_index)
{
    const auto& path_select_file_node = GetNode<Nodes::PathSelectFile>(program_index);

    SelectFileDlg select_file_dlg;

    // evaluate the title
    if( path_select_file_node.title_expression != -1 )
        select_file_dlg.SetTitle(EvaluateSharableString(path_select_file_node.title_expression));

    // evaluate whether to show directories
    select_file_dlg.SetShowDirectories(EvaluateOptionalConditional(path_select_file_node.show_directories_expression, true));

    try
    {
        // evaluate the filter
        select_file_dlg.SetFilter(PathFilterEvaluator(*this, path_select_file_node.filter_type_or_expression));
    }

    catch( const CSProException& exception )
    {
        issaerror(MessageType::Error, 100373, exception.what());
        return AssignStringNull();
    }


    // evaluate the start and root directories
    auto evaluate_directory = [&](const int directory_variant_index) -> std::optional<SpecialDirectoryLister::SpecialDirectory>
    {
        if( directory_variant_index == -1 )
            return std::nullopt;

        SpecialDirectoryLister::SpecialDirectory special_directory = DirectoryVariantEvaluator<true>(*this, directory_variant_index);

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
                start_directory = UTF8_TODO::GetUtf8(GetFilePath(m_pEngineDriver->m_pPifFile->GetAppFName()));
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

            catch( const CSProException& )
            {
                // match pre-8.0 behavior where the root directory will be set to the start directory (with a warning) when possible
                if( start_directory->index() == root_directory->index() )
                {
                    issaerror(MessageType::Warning, 100374, SpecialDirectoryLister::GetSpecialDirectoryPath(*start_directory).c_str(),
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

    catch( const CSProException& exception )
    {
        issaerror(MessageType::Error, 100372, exception.what());
        return AssignStringNull();
    }


    // show the dialog
    select_file_dlg.DoModalOnUIThread();

    return AssignString(select_file_dlg.GetSelectedPath());
}


double CIntDriver::exdirlist(const int program_index)
{
    const auto& dirlist_node = GetNode<Nodes::DirList>(program_index);

    LogicList& logic_list = GetSymbolLogicList(dirlist_node.list_symbol_index);

    if( logic_list.IsReadOnly() )
    {
        issaerror(MessageType::Error, MGF::List_read_only_cannot_be_modified_965, logic_list.GetName().c_str());
        return DEFAULT;
    }

    logic_list.Reset();

    try
    {
        const SpecialDirectoryLister::SpecialDirectory special_directory = DirectoryVariantEvaluator<true>(*this, dirlist_node.directory_variant_index);
        const SharableString filter = PathFilterEvaluator(*this, dirlist_node.filter_type_or_expression);
        const bool recursive = EvaluateOptionalConditional(dirlist_node.recursive_expression, false);
        constexpr bool include_files = true;

        // process regular directories
        if( std::holds_alternative<std::string>(special_directory) )
        {
            if( !PortableFunctions::FileIsDirectory(std::get<std::string>(special_directory)) )
                return 0;

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

            std::unique_ptr<SpecialDirectoryLister> special_directory_lister = SpecialDirectoryLister::CreateSpecialDirectoryLister(
                std::get<SpecialDirectoryLister::MediaStoreDirectory>(special_directory),
                recursive_override, include_files, include_directories);
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

    catch( const CSProException& exception )
    {
        issaerror(MessageType::Error, 100373, exception.what());
        return 0;
    }

    return 1;
}
