#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include "Engine.h"
#include <zEngineO/EngineDictionary.h>
#include <zEngineO/File.h>
#include <zEngineO/List.h>
#include <zEngineO/Versioning.h>
#include <zEngineO/Nodes/Path.h>
#include <zPlatformO/PlatformInterface.h>
#include <zToolsO/FileIO.h>
#include <zUtilO/CSProExecutables.h>
#include <zUtilO/SpecialDirectoryLister.h>
#include <zUtilF/SelectFileDlg.h>
#include <ZBRIDGEO/npff.h>


namespace
{
    constexpr wstring_view InvalidPathText = _T("<invalid path>");


    template<bool AllowSpecialDirectories, typename CN,
             typename RV = typename std::conditional<AllowSpecialDirectories, SpecialDirectoryLister::SpecialDirectory, std::wstring>::type>
    RV DirectoryVariantEvaluator(CIntDriver* int_driver, const CN& directory_variant_index_or_node)
    {
        CEngineArea* m_pEngineArea = int_driver->m_pEngineArea;
        CEngineDriver* m_pEngineDriver = int_driver->m_pEngineDriver;

        auto GetSymbolTable = [&]() -> const Logic::SymbolTable& { return int_driver->GetSymbolTable(); };

        auto get_directory_variant_node = [&]() -> const Nodes::DirectoryVariant&
        {
            if constexpr(std::is_same_v<CN, int>)
            {
                return int_driver->GetNode<Nodes::DirectoryVariant>(directory_variant_index_or_node);
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
            std::wstring filename = int_driver->EvalAlphaExpr(directory_variant_node.code_or_expression);

            if constexpr(AllowSpecialDirectories)
            {
                if( SpecialDirectoryLister::IsSpecialDirectory(filename) )
                    return SpecialDirectoryLister::EvaluateSpecialDirectory(filename);
            }

            int_driver->MakeFullPathFileName(filename);

            return filename;
        }


        // --------------------------------------------------------------------------
        // symbols
        // --------------------------------------------------------------------------
        if( directory_variant_node.type == Nodes::DirectoryVariant::Type::Symbol )
        {
            const Symbol& symbol = NPT_Ref(directory_variant_node.code_or_expression);
            std::wstring symbol_filename;

            if( symbol.IsA(SymbolType::Dictionary) )
            {
#ifdef WIN_DESKTOP
                symbol_filename = CS2WS(assert_cast<const EngineDictionary&>(symbol).GetDictionary().GetFullFileName());
#else
                // 20131210 when opening a .pen file on the portable environment, it doesn't
                // really make sense to query where the .dcf is, so we'll return the application
                // path (which is, in some senses, where the dictionary is located)
                symbol_filename = CS2WS(m_pEngineDriver->m_pPifFile->GetAppFName());
#endif
            }

            else if( symbol.IsA(SymbolType::Pre80Dictionary) )
            {
#ifdef WIN_DESKTOP
                symbol_filename = CS2WS(assert_cast<const DICT&>(symbol).GetDataDict()->GetFullFileName());
#else
                // 20131210 when opening a .pen file on the portable environment, it doesn't
                // really make sense to query where the .dcf is, so we'll return the application
                // path (which is, in some senses, where the dictionary is located)
                symbol_filename = CS2WS(m_pEngineDriver->m_pPifFile->GetAppFName());
#endif
            }

            // a file
            else
            {
                ASSERT(symbol.IsA(SymbolType::File));
                symbol_filename = assert_cast<const LogicFile&>(symbol).GetFilename();
            }

            return CS2WS(GetFilePath(WS2CS(symbol_filename)));
        }


        // --------------------------------------------------------------------------
        // path types
        // --------------------------------------------------------------------------
        if( directory_variant_node.type == Nodes::DirectoryVariant::Type::Path )
        {
            switch( static_cast<Nodes::DirectoryVariant::PathType>(directory_variant_node.code_or_expression) )
            {
                case Nodes::DirectoryVariant::PathType::Temp:
                    return GetTempDirectory();

                case Nodes::DirectoryVariant::PathType::Application:
                    return CS2WS(GetFilePath(m_pEngineDriver->m_pPifFile->GetAppFName()));

                case Nodes::DirectoryVariant::PathType::InputFile:
                    return CS2WS(GetFilePath(m_pEngineArea->GetEngineData().dictionaries_pre80.front()->GetDicX()->GetDataRepository().GetName(DataRepositoryNameType::Full)));

                case Nodes::DirectoryVariant::PathType::CSPro:
                    return CSProExecutables::GetApplicationDirectory();

#ifdef WIN_DESKTOP
                case Nodes::DirectoryVariant::PathType::Desktop:
                    return GetWindowsSpecialFolder(WindowsSpecialFolder::Desktop);

                case Nodes::DirectoryVariant::PathType::Windows:
                    return GetWindowsSpecialFolder(WindowsSpecialFolder::Windows);

                case Nodes::DirectoryVariant::PathType::Documents:
                    return GetWindowsSpecialFolder(WindowsSpecialFolder::Documents);

                case Nodes::DirectoryVariant::PathType::ProgramFiles32:
                    return GetWindowsSpecialFolder(WindowsSpecialFolder::ProgramFiles32);

                case Nodes::DirectoryVariant::PathType::ProgramFiles64:
                    return GetWindowsSpecialFolder(WindowsSpecialFolder::ProgramFiles64);
#else
                case Nodes::DirectoryVariant::PathType::CSEntry:
                    return PlatformInterface::GetInstance()->GetCSEntryDirectory();

                case Nodes::DirectoryVariant::PathType::CSEntryExternal:
                    return CS2WS(PlatformInterface::GetInstance()->GetExternalMemoryCardDirectory());
#endif
                case Nodes::DirectoryVariant::PathType::Html:
                    return Html::GetDirectory();

                case Nodes::DirectoryVariant::PathType::Downloads:
                    return GetDownloadsFolder();

                default:
                    return std::wstring();
            }
        }


        // --------------------------------------------------------------------------
        // media types
        // --------------------------------------------------------------------------
        if constexpr(AllowSpecialDirectories)
        {
            if( directory_variant_node.type == Nodes::DirectoryVariant::Type::Media )
                return SpecialDirectoryLister::MediaStoreDirectory { static_cast<MediaStore::MediaType>(directory_variant_node.code_or_expression), std::wstring() };
        }


        return std::wstring();
    }


    std::optional<std::wstring> PathFilterEvaluator(CIntDriver* int_driver, int filter_type_or_expression)
    {
        if( filter_type_or_expression == -1 )
        {
            // no filter
        }

        else if( filter_type_or_expression >= 0 )
        {
            std::wstring filter = SpecialDirectoryLister::EvaluateFilter(int_driver->EvalAlphaExpr(filter_type_or_expression));

            if( !filter.empty() )
                return filter;
        }

        else
        {
            Nodes::Path::FilterType filter_type = static_cast<Nodes::Path::FilterType>(filter_type_or_expression);
            ASSERT(filter_type == Nodes::Path::FilterType::Audio ||
                   filter_type == Nodes::Path::FilterType::Geometry ||
                   filter_type == Nodes::Path::FilterType::Image);

            return SpecialDirectoryLister::EvaluateFilter(filter_type);
        }

        return std::nullopt;
    }
}


double CIntDriver::expathname(int iExpr)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(iExpr);

    auto create_fake_node_for_pre77 = [&]()
    {
        if( va_node.arguments[0] < 0 )
        {
            return Nodes::DirectoryVariant { Nodes::DirectoryVariant::Type::Symbol, -1 * va_node.arguments[0] };
        }

        else
        {
            return Nodes::DirectoryVariant { Nodes::DirectoryVariant::Type::Path, va_node.arguments[0] };
        }
    };

    std::wstring full_path = Versioning::MeetsCompiledLogicVersion(Serializer::Iteration_7_7_000_1) ?
        DirectoryVariantEvaluator<false>(this, va_node.arguments[0]) :
        DirectoryVariantEvaluator<false>(this, create_fake_node_for_pre77());

    if( full_path.empty() )
    {
        full_path = InvalidPathText;
    }

    else
    {
        full_path = PortableFunctions::PathEnsureTrailingSlash(full_path);
    }

    return AssignAlphaValue(std::move(full_path));
}


double CIntDriver::expathconcat(int iExpr)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(iExpr);
    int number_arguments = va_node.arguments[0];
    std::wstring full_path;
    bool check_first_argument_empty = true;

    if( Versioning::MeetsCompiledLogicVersion(Serializer::Iteration_7_7_000_1) )
    {
        full_path = DirectoryVariantEvaluator<false>(this, va_node.arguments[1]);
    }

    else if( va_node.arguments[1] == 0 ) // pre-7.7 starting with a string
    {
        check_first_argument_empty = false;
    }

    else // pre-77 starting with a path type or symbol
    {
        auto create_fake_node_for_pre77 = [&]()
        {
            if( va_node.arguments[1] < 0 )
            {
                return Nodes::DirectoryVariant { Nodes::DirectoryVariant::Type::Symbol, -1 * va_node.arguments[1] };
            }

            else
            {
                return Nodes::DirectoryVariant { Nodes::DirectoryVariant::Type::Path, va_node.arguments[1] };
            }
        };

        full_path = DirectoryVariantEvaluator<false>(this, create_fake_node_for_pre77());
    }

    // prevent invalid calls on the wrong platform, like path.concat(CSEntry, "a.txt") on Windows
    bool keep_processing = ( !check_first_argument_empty || !full_path.empty() );

    for( int i = 2; keep_processing && i <= number_arguments; ++i )
    {
        std::wstring this_path = EvalAlphaExpr(va_node.arguments[i]);

        if( full_path.empty() )
        {
            full_path = this_path;
            MakeFullPathFileName(full_path);
        }

        else
        {
            full_path = MakeFullPath(full_path, this_path);
        }
    }

    if( full_path.empty() )
        full_path = InvalidPathText;

    return AssignAlphaValue(std::move(full_path));
}


double CIntDriver::expathgetdirectoryname(int iExpr)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(iExpr);
    std::wstring path = EvalFullPathFileName(va_node.arguments[0]);
    return AssignAlphaValue(PortableFunctions::PathGetDirectory(path));
}


double CIntDriver::expathgetextension(int iExpr)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(iExpr);
    std::wstring path = EvalAlphaExpr(va_node.arguments[0]);
    return AssignAlphaValue(PortableFunctions::PathGetFileExtension(path, true));
}


double CIntDriver::expathgetfilename(int iExpr)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(iExpr);
    std::wstring path = EvalAlphaExpr(va_node.arguments[0]);
    return AssignAlphaValue(std::wstring(PortableFunctions::PathGetFilename(path)));
}


double CIntDriver::expathgetfilenamewithoutextension(int iExpr)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(iExpr);
    std::wstring path = EvalAlphaExpr(va_node.arguments[0]);
    return AssignAlphaValue(PortableFunctions::PathGetFilenameWithoutExtension(path));
}


double CIntDriver::expathgetrelativepath(int iExpr)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(iExpr);
    std::wstring relative_to = DirectoryVariantEvaluator<false>(this, va_node.arguments[0]);

    std::wstring relative_path = EvalFullPathFileName(va_node.arguments[1]);

    if( !relative_to.empty() )
    {
        // GetRelativeFNameForDisplay expects a file, not a directory, so create a fake filename
        relative_to = PortableFunctions::PathEnsureTrailingSlash(relative_to) + _T("g");
        relative_path = GetRelativeFNameForDisplay(relative_to, relative_path);
    }

    return AssignAlphaValue(std::move(relative_path));
}


double CIntDriver::expathselectfile(int iExpr)
{
    const auto& path_select_file_node = GetNode<Nodes::PathSelectFile>(iExpr);

    SelectFileDlg select_file_dlg;

    // evaluate the title
    if( path_select_file_node.title_expression != -1 )
        select_file_dlg.SetTitle(EvalAlphaExpr(path_select_file_node.title_expression));


    // evaluate whether to show directories
    select_file_dlg.SetShowDirectories(ConditionalValueIsTrue(
        EvaluateOptionalNumericExpression(path_select_file_node.show_directories_expression, 1)));

    try
    {
        // evaluate the filter
        std::optional<std::wstring> filter = PathFilterEvaluator(this, path_select_file_node.filter_type_or_expression);

        if( filter.has_value() )
            select_file_dlg.SetFilter(std::move(*filter));
    }

    catch( const CSProException& exception )
    {
        issaerror(MessageType::Error, 100373, exception.GetErrorMessage().c_str());
        return AssignBlankAlphaValue();
    }


    // evaluate the start and root directories
    auto evaluate_directory = [&](int directory_variant_index) -> std::optional<SpecialDirectoryLister::SpecialDirectory>
    {
        if( directory_variant_index == -1 )
            return std::nullopt;

        SpecialDirectoryLister::SpecialDirectory special_directory = DirectoryVariantEvaluator<true>(this, directory_variant_index);

        if( std::holds_alternative<std::wstring>(special_directory) )
        {
            if( PortableFunctions::FileIsDirectory(std::get<std::wstring>(special_directory)) )
                return PortableFunctions::PathRemoveTrailingSlash(std::get<std::wstring>(special_directory));

            throw FileIO::Exception::DirectoryNotFound(std::get<std::wstring>(special_directory));
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
                start_directory = CS2WS(GetFilePath(m_pEngineDriver->m_pPifFile->GetAppFName()));
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
        issaerror(MessageType::Error, 100372, exception.GetErrorMessage().c_str());
        return AssignBlankAlphaValue();
    }


    // show the dialog
    select_file_dlg.DoModalOnUIThread();

    return AssignAlphaValue(select_file_dlg.GetSelectedPath());
}


double CIntDriver::exdirlist(int program_index)
{
    if( Versioning::PredatesCompiledLogicVersion(Serializer::Iteration_7_7_000_1) )
        return exdirlist_pre77(program_index);

    const auto& dirlist_node = GetNode<Nodes::DirList>(program_index);

    LogicList& logic_list = GetSymbolLogicList(dirlist_node.list_symbol_index);

    if( logic_list.IsReadOnly() )
    {
        issaerror(MessageType::Error, 965, logic_list.GetName().c_str());
        return DEFAULT;
    }

    logic_list.Reset();

    try
    {
        const SpecialDirectoryLister::SpecialDirectory special_directory = DirectoryVariantEvaluator<true>(this, dirlist_node.directory_variant_index);
        const std::optional<std::wstring> filter = PathFilterEvaluator(this, dirlist_node.filter_type_or_expression);
        bool recursive = EvaluateOptionalConditionalExpression(dirlist_node.recursive_expression, false);
        constexpr bool include_files = true;

        // process regular directories
        if( std::holds_alternative<std::wstring>(special_directory) )
        {
            if( !PortableFunctions::FileIsDirectory(std::get<std::wstring>(special_directory)) )
                return 0;

            constexpr bool include_directories = true;

            DirectoryLister directory_lister(recursive, include_files, include_directories);

            if( filter.has_value() )
                directory_lister.SetNameFilter(*filter);

            logic_list.AddStrings(directory_lister.GetPaths(std::get<std::wstring>(special_directory)));
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

            if( filter.has_value() )
                special_directory_lister->SetNameFilter(*filter);            

            logic_list.AddStrings(special_directory_lister->GetSpecialPaths());
        }

        else
        {
            ASSERT(std::holds_alternative<SpecialDirectoryLister::AndroidRoot>(special_directory));
        }
    }

    catch( const CSProException& exception )
    {
        issaerror(MessageType::Error, 100373, exception.GetErrorMessage().c_str());
        return 0;
    }

    return 1;
}


double CIntDriver::exdirlist_pre77(int iExpr)
{
    const FNN_NODE* pfun = (FNN_NODE*)PPT(iExpr);
    LogicList* logic_list = &GetSymbolLogicList(pfun->fn_expr[0]);
    std::wstring directory = EvalFullPathFileName(pfun->fn_expr[1]);

    if( logic_list->IsReadOnly() )
    {
        issaerror(MessageType::Error, 965, logic_list->GetName().c_str());
        return DEFAULT;
    }

    logic_list->Reset();

    DirectoryLister directory_lister(false, true, true);

    if( pfun->fn_expr[2] != 0 )
        directory_lister.SetRecursive();

    if( pfun->fn_nargs == 4 ) // they specified a filter
        directory_lister.SetNameFilter(EvalAlphaExpr(pfun->fn_expr[3]));

    if( !PortableFunctions::FileIsDirectory(directory) )
        return 0;

    logic_list->AddStrings(directory_lister.GetPaths(directory));

    return 1;
}
